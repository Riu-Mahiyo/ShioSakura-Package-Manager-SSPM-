// plugin/plugin_loader.cpp — 插件加载器
#include "plugin_loader.h"
#include "../core/exec_engine.h"
#include "../core/logger.h"

#include <filesystem>

namespace SSPM {
namespace fs = std::filesystem;

std::string PluginLoader::layerDir()  { return "/usr/lib/sspm/layers/";  }
std::string PluginLoader::pluginDir() { return "/usr/lib/sspm/plugins/"; }

// Phase 3 stub：扫描 .so 文件
std::vector<std::string> PluginLoader::scanLayers(const std::string& dir) {
    std::vector<std::string> result;
    if (!fs::exists(dir)) return result;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".so") {
            result.push_back(entry.path().string());
        }
    }
    return result;
}

std::vector<std::string> PluginLoader::scanPlugins(const std::string& dir) {
    std::vector<std::string> result;
    if (!fs::exists(dir)) return result;
    for (auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.path().extension() == ".so") {
            result.push_back(entry.path().string());
        }
    }
    return result;
}

// Minimal 策略：检测所有语言运行时（清单 3.3）
PluginLangStatus PluginLoader::checkLanguage(const std::string& lang) {
    PluginLangStatus s;
    s.language = lang;

    struct LangDef {
        const char* lang;
        const char* cmd;        // 主要命令
        const char* altCmd;     // 备选命令
        const char* versionArg;
    };

    static const LangDef kLangs[] = {
        {"bash",   "bash",   nullptr,   "--version"},
        {"c",      "gcc",    "clang",   "--version"},
        {"java",   "java",   nullptr,   "-version"},
        {"node",   "node",   "nodejs",  "--version"},
        {"python", "python3","python",  "--version"},
        {"go",     "go",     nullptr,   "version"},
        {"rust",   "rustc",  nullptr,   "--version"},
        {nullptr,  nullptr,  nullptr,   nullptr}
    };

    for (auto* d = kLangs; d->lang; ++d) {
        if (lang != d->lang) continue;

        if (ExecEngine::exists(d->cmd)) {
            s.available    = true;
            s.interpreter  = d->cmd;
            return s;
        }
        if (d->altCmd && ExecEngine::exists(d->altCmd)) {
            s.available    = true;
            s.interpreter  = d->altCmd;
            return s;
        }
        s.available = false;
        s.reason    = std::string(d->cmd) + " not found in PATH";
        return s;
    }

    s.available = false;
    s.reason    = "unknown language: " + lang;
    return s;
}

std::vector<PluginLangStatus> PluginLoader::checkLanguages() {
    std::vector<std::string> langs = {
        "bash", "c", "java", "node", "python", "go", "rust"
    };
    std::vector<PluginLangStatus> result;
    for (auto& l : langs) {
        result.push_back(checkLanguage(l));
    }
    return result;
}

} // namespace SSPM
