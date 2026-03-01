#pragma once
// ═══════════════════════════════════════════════════════════
//  plugin/plugin_loader.h — 插件加载器（清单 2.2）
//
//  Phase 1/2：编译进来的 C++ backend，不用 dlopen
//  Phase 3：此模块实现 dlopen 扫描 /usr/lib/sspm/layers/*.so
//
//  Minimal 系统策略（清单 3.3）：
//    - 只保证 C 和 Bash 插件
//    - 其他语言：运行前检测解释器，缺失则禁用并报告
// ═══════════════════════════════════════════════════════════
#include "plugin_api.h"
#include <string>
#include <vector>
#include <memory>

namespace SSPM {

// 插件语言检测（Minimal 策略用）
struct PluginLangStatus {
    std::string language;
    bool        available;
    std::string interpreter;  // 解释器路径
    std::string reason;       // 不可用原因
};

class PluginLoader {
public:
    // 扫描 Layer 插件目录，加载所有可用 .so
    // Phase 3 实现，现在是 stub
    static std::vector<std::string> scanLayers(
        const std::string& dir = "/usr/lib/sspm/layers/");

    // 扫描 Plugin 目录
    static std::vector<std::string> scanPlugins(
        const std::string& dir = "/usr/lib/sspm/plugins/");

    // Minimal 策略：检测所有插件语言环境（清单 3.3）
    static std::vector<PluginLangStatus> checkLanguages();

    // 检测特定语言
    static PluginLangStatus checkLanguage(const std::string& lang);

    // Layer 插件目录
    static std::string layerDir();
    static std::string pluginDir();
};

} // namespace SSPM
