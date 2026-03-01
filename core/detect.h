#pragma once
// ═══════════════════════════════════════════════════════════
//  core/detect.h — OS / backend 检测
//  取自 v1.9 detect.h，修掉：
//    ✗ LibcType（v1 detect.cpp 里有悬空 pclose bug 和 if(false) 代码）
//    ✗ 对 runtime/profile.h 的循环依赖
//  保留：
//    ✓ HostOS enum（完整的 OS 枚举）
//    ✓ detectOS() 通过 utsname + /etc/os-release
//    ✓ detectArch() 通过 utsname
//    ✓ hasCommand() 通过 ExecEngine::exists（安全）
//    ✓ isLinux() / isBSD() / isDarwin() 平台判断
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>

namespace SSPM {

enum class HostOS {
    ArchLinux,
    Debian,
    Ubuntu,
    Fedora,
    OpenSUSE,
    Gentoo,
    FreeBSD,
    OpenBSD,
    NetBSD,
    DragonFly,
    MacOS,
    Unknown,
};

class Detect {
public:
    // 启动时调用一次，初始化缓存
    static void run();

    // 访问器
    static HostOS      os();
    static std::string arch();
    static std::string osName();

    // 命令存在检测（通过 ExecEngine::exists，不走 shell）
    static bool hasCommand(const std::string& cmd);

    // 网络检测（ping 8.8.8.8）
    static bool networkReachable();

    // root 检测
    static bool isRoot();

    // 平台判断
    static bool isLinux();
    static bool isBSD();
    static bool isDarwin();

    // 自动选择默认 backend（apt / pacman / dnf / zypper / portage）
    static std::string defaultBackend();

    // 所有可用 backend 列表
    static std::vector<std::string> availableBackends();

private:
    static HostOS      os_;
    static std::string arch_;

    static HostOS      detectOS();
    static std::string detectArch();
};

} // namespace SSPM
