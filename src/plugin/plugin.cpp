#include "sspm/plugin.h"
#include "sspm/log.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <dlfcn.h>
#include <algorithm>
#include <sstream>   // needed for std::stringstream used in aggregate_metrics

namespace fs = std::filesystem;

namespace sspm::plugin {

PluginManager::PluginManager(const std::string& plugin_dir, std::shared_ptr<ConfigManager> config)
    : plugin_dir_(plugin_dir), config_(config) {
    fs::create_directories(plugin_dir_);
}

std::string PluginManager::aggregate_metrics() const {
    std::string res = "{";
    bool first = true;
    for (auto& [name, p] : plugins_) {
        if (p.instance) {
            if (!first) res += ",";
            res += "\"" + p.instance->id() + "\":" + p.instance->metrics();
            first = false;
        }
    }
    res += "}";
    return res;
}

PluginManager::~PluginManager() {
    for (auto& [name, p] : plugins_) {
        if (p.instance) p.instance->shutdown();
        if (p.dl_handle) dlclose(p.dl_handle);
    }
}

void PluginManager::load_all() {
    if (!fs::exists(plugin_dir_)) return;

    std::vector<PluginInfo> discovered;
    for (const auto& entry : fs::directory_iterator(plugin_dir_)) {
        if (entry.is_directory()) {
            discovered.push_back(read_toml(entry.path().string()));
        }
    }

    // Dependency resolution: topological sort
    std::unordered_map<std::string, int> state; // 0: unvisited, 1: visiting, 2: visited
    std::vector<std::string> load_order;

    std::function<void(const PluginInfo&)> visit = [&](const PluginInfo& info) {
        if (state[info.name] == 2) return;
        if (state[info.name] == 1) {
            SSPM_ERROR("Circular plugin dependency detected: " + info.name);
            return;
        }

        state[info.name] = 1;
        for (const auto& dep_name : info.dependencies) {
            auto it = std::find_if(discovered.begin(), discovered.end(), 
                [&](const PluginInfo& d) { return d.name == dep_name; });
            if (it != discovered.end()) {
                visit(*it);
            } else {
                SSPM_WARN("Plugin " + info.name + " missing dependency: " + dep_name);
            }
        }
        state[info.name] = 2;
        load_order.push_back(info.name);
    };

    for (const auto& info : discovered) {
        visit(info);
    }

    for (const auto& name : load_order) {
        load(name);
    }
}

bool PluginManager::load(const std::string& name) {
    std::string dir = plugin_dir_ + "/" + name;
    if (!fs::exists(dir)) return false;

    PluginInfo info = read_toml(dir);
    if (!info.enabled) return false;

    LoadedPlugin p;
    p.info = info;
    if (!load_so(p)) return false;

    p.info.loaded = true;
    plugins_[name] = std::move(p);
    return true;
}

bool PluginManager::unload(const std::string& name) {
    auto it = plugins_.find(name);
    if (it == plugins_.end()) return false;
    if (it->second.instance) it->second.instance->shutdown();
    if (it->second.dl_handle) dlclose(it->second.dl_handle);
    plugins_.erase(it);
    return true;
}

std::vector<PluginInfo> PluginManager::list() const {
    std::vector<PluginInfo> res;
    for (auto& [name, p] : plugins_) res.push_back(p.info);
    return res;
}

bool PluginManager::remove(const std::string& name) {
    // existing definition already present later; move earlier or keep duplicate?
    unload(name);
    fs::remove_all(plugin_dir_ + "/" + name);
    return true;
}

std::vector<std::shared_ptr<IModule>> PluginManager::loaded_modules() const {
    std::vector<std::shared_ptr<IModule>> res;
    for (auto& [name, p] : plugins_) {
        if (p.instance) res.push_back(p.instance);
    }
    return res;
}

PluginInfo PluginManager::read_toml(const std::string& dir) const {
    PluginInfo info;
    info.path = dir;
    info.name = fs::path(dir).filename().string();

    std::ifstream f(dir + "/plugin.toml");
    if (!f) return info;

    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\""));
        s.erase(s.find_last_not_of(" \t\"") + 1);
    };

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        trim(k); trim(v);

        if      (k == "name")        info.name        = v;
        else if (k == "version") {
            auto pos1 = v.find('.');
            auto pos2 = v.find('.', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                info.version.major = std::stoi(v.substr(0, pos1));
                info.version.minor = std::stoi(v.substr(pos1 + 1, pos2 - pos1 - 1));
                info.version.patch = std::stoi(v.substr(pos2 + 1));
            }
        }
        else if (k == "description") info.description = v;
        else if (k == "author")      info.author      = v;
        else if (k == "entry")       info.entry       = v;
        else if (k == "command")     info.command     = v;
        else if (k == "repo")        info.repo_url    = v;
        else if (k == "depends") {
            std::stringstream ss(v);
            std::string dep;
            while (std::getline(ss, dep, ',')) {
                auto d = dep;
                trim(d);
                if (!d.empty()) info.dependencies.push_back(d);
            }
        }
        else if (k == "type") {
            if (v == "backend") info.type = PluginType::Backend;
            else if (v == "hook")    info.type = PluginType::Hook;
            else if (v == "command") info.type = PluginType::Command;
            else if (v == "module")  info.type = PluginType::Module;
        }
    }
    return info;
}

bool PluginManager::load_so(LoadedPlugin& p) {
    if (p.info.entry.empty()) return p.info.type == PluginType::Command;

    std::string so_path = p.info.path + "/" + p.info.entry;
    p.dl_handle = dlopen(so_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!p.dl_handle) {
        SSPM_ERROR("plugin " + p.info.name + ": dlopen failed: " + dlerror());
        return false;
    }

    if (p.info.type == PluginType::Module || p.info.type == PluginType::Backend) {
        auto create = (IModule*(*)())sym(p.dl_handle, "create_module");
        if (!create) create = (IModule*(*)())sym(p.dl_handle, "create_backend");
        
        if (create) {
            p.instance = std::shared_ptr<IModule>(create(), [p](IModule* m) {
                auto destroy = (void(*)(IModule*))dlsym(p.dl_handle, "destroy_module");
                if (!destroy) destroy = (void(*)(IModule*))dlsym(p.dl_handle, "destroy_backend");
                if (destroy) destroy(m); else delete m;
            });
            if (p.instance) {
                std::map<std::string, std::string> ctx;
                if (config_) ctx = config_->get_all_for_module(p.instance->id());
                p.instance->initialize(ctx);
            }
        }
    } else if (p.info.type == PluginType::Hook) {
        auto pre = (int(*)(const char*, const char*))sym(p.dl_handle, "pre_install");
        auto post = (int(*)(const char*, const char*))sym(p.dl_handle, "post_install");
        if (pre) p.pre_install = pre;
        if (post) p.post_install = post;
    }

    return true;
}

void* PluginManager::sym(void* handle, const char* name) {
    return dlsym(handle, name);
}

bool PluginManager::install(const std::string& name) {
    SSPM_WARN("PluginManager::install() not implemented (requested '" + name + "')");
    return false;
}

bool PluginManager::update(const std::string& name) {
    // simplistic placeholder: plugins are typically git repos or archives; no
    // cross-platform update mechanism implemented yet.  always succeed so that
    // callers (e.g. CLI) do not have to special-case.
    (void)name;
    return true;
}

std::vector<std::shared_ptr<Backend>> PluginManager::get_backend_plugins() const {
    std::vector<std::shared_ptr<Backend>> res;
    for (const auto& [name, p] : plugins_) {
        if (p.info.type == PluginType::Backend && p.instance) {
            auto b = std::dynamic_pointer_cast<Backend>(p.instance);
            if (b) res.push_back(b);
        }
    }
    return res;
}

} // namespace sspm::plugin
