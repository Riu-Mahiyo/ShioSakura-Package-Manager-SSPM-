#pragma once
// ═══════════════════════════════════════════════════════════
//  core/backend_setup.h — Backend 安装向导
//  架构文档第三节 Backend 列表：
//    "增加 backend 是否已安装的功能，如果没有的话
//     可以询问用户是否需要一键帮安装并自动配置好"
//
//  功能：
//    1. 检测 backend 是否已安装
//    2. 交互式询问用户是否安装
//    3. 一键安装并配置镜像源
//    4. macOS 版本检测（brew 限制老版本，macports 支持）
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include <functional>

namespace SSPM {

struct BackendInstallInfo {
    std::string name;
    std::string description;
    std::string checkCmd;          // 检测命令
    std::string installCmd;        // 安装方法描述
    bool        requiresRoot;
    bool        availableOnLinux;
    bool        availableOnMacOS;
    bool        availableOnFreeBSD;
    std::string minMacOSVersion;   // 最低 macOS 版本（空=无限制）
};

class BackendSetup {
public:
    // 检测 backend 是否已安装
    static bool isInstalled(const std::string& backend);

    // 交互式安装向导
    // 若 backend 未安装，询问用户并自动安装
    // 返回：是否最终可用
    static bool ensureAvailable(const std::string& backend,
                                 bool interactive = true);

    // 批量检查并询问
    static void checkAndPromptAll(const std::vector<std::string>& backends);

    // 一键安装 backend（非交互式，用于 CI）
    static bool install(const std::string& backend);

    // 获取 backend 安装信息（用于展示）
    static const BackendInstallInfo* getInfo(const std::string& backend);

    // macOS 版本检测（影响 brew/macports 可用性）
    static std::string getMacOSVersion();
    static bool isMacOSVersionOldEnoughForBrewBlock();  // < 12 视为"老版本"
    static bool shouldSuggestMacPortsForOldMacOS();

    // 为老版本 macOS 用户安装 MacPorts .pkg
    static bool installMacPortsForOldMacOS();

private:
    static std::vector<BackendInstallInfo> buildDatabase();
    static const std::vector<BackendInstallInfo>& db();

    // 执行实际安装
    static bool doInstallBrew();
    static bool doInstallFlatpak();
    static bool doInstallSnap();
    static bool doInstallNix();
    static bool doInstallMacPorts();
    static bool doInstallCargo();
    static bool doInstallGem();

    // 交互确认
    static bool askUser(const std::string& prompt);
};

} // namespace SSPM
