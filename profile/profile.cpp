// ═══════════════════════════════════════════════════════════
//  profile/profile.cpp — 配置 Profile 系统实现
// ═══════════════════════════════════════════════════════════
#include "profile.h"
#include "../core/logger.h"
#include "../layer/layer_manager.h"
#include "../db/skydb.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>

namespace SSPM {
namespace fs = std::filesystem;

// ── Profile 目录 ──────────────────────────────────────────
std::string ProfileManager::profileDir() {
    const char* xdg = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");
    std::string base = xdg ? std::string(xdg) : (home ? std::string(home) + "/.config" : "/tmp");
    return base + "/sspm/profiles";
}

// ── 极简 TOML 解析器 ──────────────────────────────────────
Profile ProfileManager::parseToml(const std::string& content) {
    Profile profile;
    std::string currentSection;
    std::string currentBackend;

    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        // 去掉注释
        auto comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);

        // trim
        auto trim = [](std::string s) {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s = s.substr(1);
            while (!s.empty() && (s.back()  == ' ' || s.back()  == '\t')) s.pop_back();
            return s;
        };
        line = trim(line);
        if (line.empty()) continue;

        // [section]
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            trim(currentSection);
            continue;
        }

        // key = value 或 key = ["a", "b"]
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (currentSection == "meta") {
            // 去掉引号
            auto stripQuotes = [](std::string s) {
                if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                    return s.substr(1, s.size() - 2);
                return s;
            };
            if (key == "name")        profile.name    = stripQuotes(val);
            if (key == "version")     profile.version = stripQuotes(val);
            if (key == "description") profile.description = stripQuotes(val);
            if (key == "author")      profile.author  = stripQuotes(val);
        }
        else if (currentSection == "packages") {
            // val 格式：["curl", "git", ...]
            if (val.front() == '[') {
                std::vector<std::string> pkgs;
                std::string inner = val.substr(1, val.size() - 2);
                std::istringstream items(inner);
                std::string item;
                while (std::getline(items, item, ',')) {
                    // 去掉空格和引号
                    while (!item.empty() && (item.front() == ' ' || item.front() == '"')) item = item.substr(1);
                    while (!item.empty() && (item.back()  == ' ' || item.back()  == '"')) item.pop_back();
                    if (!item.empty()) pkgs.push_back(item);
                }
                profile.packages[key] = pkgs;
            }
        }
        else if (currentSection == "settings") {
            auto stripQuotes = [](std::string s) {
                if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                    return s.substr(1, s.size() - 2);
                return s;
            };
            if (key == "mirror_region")  profile.settings.mirrorRegion  = stripQuotes(val);
            if (key == "allow_root")     profile.settings.allowRoot      = (val == "true");
            if (key == "dry_run_first")  profile.settings.dryRunFirst    = (val != "false");
        }
    }

    return profile;
}

// ── TOML 序列化 ───────────────────────────────────────────
std::string ProfileManager::dumpToml(const Profile& p) {
    std::ostringstream ss;
    ss << "# SSPM Profile - 可复现安装配置\n\n";

    ss << "[meta]\n";
    ss << "name        = \"" << p.name << "\"\n";
    ss << "version     = \"" << (p.version.empty() ? "1.0" : p.version) << "\"\n";
    if (!p.description.empty())
        ss << "description = \"" << p.description << "\"\n";
    ss << "\n";

    ss << "[packages]\n";
    for (auto& [backend, pkgs] : p.packages) {
        if (pkgs.empty()) continue;
        ss << backend << " = [";
        for (size_t i = 0; i < pkgs.size(); ++i) {
            ss << "\"" << pkgs[i] << "\"";
            if (i + 1 < pkgs.size()) ss << ", ";
        }
        ss << "]\n";
    }
    ss << "\n";

    ss << "[settings]\n";
    if (!p.settings.mirrorRegion.empty())
        ss << "mirror_region = \"" << p.settings.mirrorRegion << "\"\n";
    ss << "allow_root    = " << (p.settings.allowRoot ? "true" : "false") << "\n";
    ss << "dry_run_first = " << (p.settings.dryRunFirst ? "true" : "false") << "\n";

    return ss.str();
}

// ── 加载/保存 ─────────────────────────────────────────────
Profile ProfileManager::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        Logger::error("无法读取 profile: " + path);
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return parseToml(content);
}

bool ProfileManager::save(const Profile& profile, const std::string& path) {
    try {
        fs::create_directories(fs::path(path).parent_path());
    } catch (...) {}

    std::ofstream f(path);
    if (!f) return false;
    f << dumpToml(profile);
    return true;
}

// ── 导出当前状态 ──────────────────────────────────────────
Profile ProfileManager::exportCurrent(const std::string& name) {
    Profile p;
    p.name = name;
    p.version = "1.0";
    p.description = "Exported from current system";

    // 从 SkyDB 读取已安装包
    SkyDB::load();
    auto allPkgs = SkyDB::all();
    for (auto& [pkgName, rec] : allPkgs) {
        p.packages[rec.backend].push_back(pkgName);
    }

    return p;
}

// ── 应用 Profile（declarative install）───────────────────
int ProfileManager::apply(const Profile& profile, bool dryRun) {
    int ops = 0;

    if (dryRun) {
        Logger::info("[profile] dry-run 模式 — 仅预览，不实际执行");
        std::cout << "\033[1m将要安装以下包：\033[0m\n";
    }

    for (auto& [backend, pkgs] : profile.packages) {
        if (pkgs.empty()) continue;

        auto* be = LayerManager::getAvailable(backend);
        if (!be) {
            Logger::warn("[profile] backend '" + backend + "' 不可用，跳过 " +
                         std::to_string(pkgs.size()) + " 个包");
            continue;
        }

        // 过滤已安装的包
        std::vector<std::string> toInstall;
        for (auto& pkg : pkgs) {
            auto ver = be->installedVersion(pkg);
            if (ver.empty()) {
                toInstall.push_back(pkg);
                if (dryRun) {
                    std::cout << "  + " << pkg << " \033[0;90m(via " << backend << ")\033[0m\n";
                }
            }
        }

        if (toInstall.empty()) continue;

        if (!dryRun) {
            Logger::step("[profile] 安装 " + std::to_string(toInstall.size()) +
                        " 个包 (backend=" + backend + ")");
            auto result = be->install(toInstall);
            if (result.ok()) ops += (int)toInstall.size();
            else Logger::error("[profile] 部分包安装失败: " + result.errorMessage);
        } else {
            ops += (int)toInstall.size();
        }
    }

    if (dryRun) {
        std::cout << "\n共 " << ops << " 个包待安装。使用 --apply 实际执行\n";
    }

    return ops;
}

// ── Profile Diff ──────────────────────────────────────────
ProfileManager::Diff ProfileManager::diff(const Profile& current,
                                            const Profile& target) {
    Diff result;

    // 构建当前包集合
    std::map<std::string, std::string> currentSet;  // name → backend
    for (auto& [backend, pkgs] : current.packages) {
        for (auto& pkg : pkgs) currentSet[pkg] = backend;
    }

    // 构建目标包集合
    std::map<std::string, std::string> targetSet;
    for (auto& [backend, pkgs] : target.packages) {
        for (auto& pkg : pkgs) targetSet[pkg] = backend;
    }

    // 需要安装（目标有，当前没有）
    for (auto& [pkg, backend] : targetSet) {
        if (!currentSet.count(pkg)) {
            ProfilePkg pp;
            pp.name = pkg; pp.backend = backend;
            result.toInstall.push_back(pp);
        } else {
            ProfilePkg pp;
            pp.name = pkg; pp.backend = backend;
            result.unchanged.push_back(pp);
        }
    }

    // 需要移除（当前有，目标没有）
    for (auto& [pkg, backend] : currentSet) {
        if (!targetSet.count(pkg)) {
            ProfilePkg pp;
            pp.name = pkg; pp.backend = backend;
            result.toRemove.push_back(pp);
        }
    }

    return result;
}

std::vector<std::string> ProfileManager::listProfiles() {
    std::vector<std::string> result;
    std::string dir = profileDir();
    if (!fs::exists(dir)) return result;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".profile") {
            result.push_back(entry.path().stem().string());
        }
    }
    return result;
}

} // namespace SSPM
