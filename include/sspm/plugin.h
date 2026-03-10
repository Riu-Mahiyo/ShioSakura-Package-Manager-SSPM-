#pragma once
#include "sspm/backend.h"
#include "sspm/module.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace sspm::plugin {

// ── Plugin types ──────────────────────────────────────────────────────────────
enum class PluginType {
    Backend,    // provides a new Backend (e.g. AUR, brew tap, Docker)
    Hook,       // lifecycle hooks (pre/post install/remove)
    Command,    // adds extra CLI commands (e.g. sspm aur search)
    Module,     // generic system module
    Unknown
};

// ── Plugin Info ───────────────────────────────────────────────────────────────
struct PluginInfo {
    std::string name;
    Version version;
    std::string description;
    std::string author;
    PluginType  type      = PluginType::Unknown;
    std::string entry;     // .so filename
    std::string command;   // CLI subcommand name
    std::string repo_url;  // origin URL for update checks
    std::string path;      // absolute path to plugin directory
    std::vector<std::string> dependencies;
    bool loaded           = false;
    bool enabled          = true;
};

// ── Loaded Plugin Instance ───────────────────────────────────────────────────
struct LoadedPlugin {
    PluginInfo info;
    void* dl_handle = nullptr;
    std::shared_ptr<IModule> instance;
    
    // Function pointers for legacy hook/command plugins
    std::function<int(const char*, const char*)> pre_install;
    std::function<int(const char*, const char*)> post_install;
};

class PluginManager {
public:
    explicit PluginManager(const std::string& plugin_dir, std::shared_ptr<ConfigManager> config = nullptr);
    ~PluginManager();

    // Load all installed plugins at startup
    void load_all();

    // Load a specific plugin by name
    bool load(const std::string& name);

    // Unload a plugin (dlclose)
    bool unload(const std::string& name);

    // List all plugins (loaded or not)
    std::vector<PluginInfo> list() const;

    // Get all loaded module instances
    std::vector<std::shared_ptr<IModule>> loaded_modules() const;

    // Install a plugin by name (e.g. fetch from registry).  stub returns
    // false until a real implementation is provided.
    bool install(const std::string& name);

    // Remove a plugin from disk (unloads first)
    bool remove(const std::string& name);

    // Refresh / update plugin sources (stub on platforms without a package)
    bool update(const std::string& name = "");

    // Return backend instances provided by loaded backend-type plugins.
    std::vector<std::shared_ptr<Backend>> get_backend_plugins() const;

    /** @return Aggregated metrics from all loaded modules (JSON format). */
    std::string aggregate_metrics() const;

private:
    std::string plugin_dir_;
    std::shared_ptr<ConfigManager> config_;
    std::unordered_map<std::string, LoadedPlugin> plugins_;

    PluginInfo read_toml(const std::string& dir) const;
    bool load_so(LoadedPlugin& p);
    void* sym(void* handle, const char* name);
};

// note: previously there were a number of stray declarations and a second
// "LoadedPlugin" struct here. they were accidentally placed outside of the
// PluginManager class and resulted in compiler errors.  the relevant
// functionality either lives in PluginManager or was unused; the extraneous
// definitions are removed to restore a clean interface.

} // namespace sspm::plugin
