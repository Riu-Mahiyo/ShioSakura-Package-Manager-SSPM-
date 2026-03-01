// ═══════════════════════════════════════════════════════════
//  core/detect.cpp — OS/backend 检测实现
//
//  取自 v1.9 detect.cpp 的好逻辑：
//    ✓ /etc/os-release 解析（直接读文件，安全）
//    ✓ utsname 架构检测
//    ✓ isLinux / isBSD / isDarwin
//    ✓ osName() 字符串映射
//
//  修掉的 v1 bug：
//    ✗ detectLibc()：if(false) 悬空代码块，pclose 从未被调用
//    ✗ ExecEngine::captureLine("ldd", ...) 调用但结果未用
//    → 直接删除 LibcType，不做 libc 检测（Phase 1 不需要）
//
//  修掉的 v1 坏设计：
//    ✗ 依赖 Logger 在 run() 里打印（detect 是基础层，不应依赖 logger）
//    → 调用方（main/doctor）负责打印
// ═══════════════════════════════════════════════════════════
#include "detect.h"
#include "exec_engine.h"

#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/utsname.h>

namespace SSPM {

// 静态成员初始化
HostOS      Detect::os_   = HostOS::Unknown;
std::string Detect::arch_ = "unknown";

// ── 内部辅助 ─────────────────────────────────────────────
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ── 检测逻辑 ─────────────────────────────────────────────

HostOS Detect::detectOS() {
    struct utsname u{};
    uname(&u);
    std::string sysname = u.sysname;

    if (sysname == "Darwin")    return HostOS::MacOS;
    if (sysname == "FreeBSD")   return HostOS::FreeBSD;
    if (sysname == "OpenBSD")   return HostOS::OpenBSD;
    if (sysname == "NetBSD")    return HostOS::NetBSD;
    if (sysname == "DragonFly") return HostOS::DragonFly;

    // Linux：读 /etc/os-release
    std::string osrel = readFile("/etc/os-release");
    if (osrel.empty()) osrel = readFile("/usr/lib/os-release");

    if (osrel.find("Arch Linux") != std::string::npos ||
        osrel.find("ID=arch")    != std::string::npos)    return HostOS::ArchLinux;
    if (osrel.find("ID=debian")  != std::string::npos)    return HostOS::Debian;
    if (osrel.find("ID=ubuntu")  != std::string::npos ||
        osrel.find("ID_LIKE=ubuntu") != std::string::npos) return HostOS::Ubuntu;
    if (osrel.find("ID=fedora")  != std::string::npos)    return HostOS::Fedora;
    if (osrel.find("ID=opensuse") != std::string::npos ||
        osrel.find("ID_LIKE=suse") != std::string::npos)  return HostOS::OpenSUSE;
    if (osrel.find("ID=gentoo")  != std::string::npos)    return HostOS::Gentoo;

    return HostOS::Unknown;
}

std::string Detect::detectArch() {
    struct utsname u{};
    uname(&u);
    return u.machine;
}

// ── 公开接口 ─────────────────────────────────────────────

void Detect::run() {
    os_   = detectOS();
    arch_ = detectArch();
}

HostOS      Detect::os()   { return os_; }
std::string Detect::arch() { return arch_; }

std::string Detect::osName() {
    switch (os_) {
        case HostOS::ArchLinux: return "Arch Linux";
        case HostOS::Debian:    return "Debian";
        case HostOS::Ubuntu:    return "Ubuntu";
        case HostOS::Fedora:    return "Fedora";
        case HostOS::OpenSUSE:  return "openSUSE";
        case HostOS::Gentoo:    return "Gentoo";
        case HostOS::FreeBSD:   return "FreeBSD";
        case HostOS::OpenBSD:   return "OpenBSD";
        case HostOS::NetBSD:    return "NetBSD";
        case HostOS::DragonFly: return "DragonFlyBSD";
        case HostOS::MacOS:     return "macOS";
        default:                return "Unknown";
    }
}

bool Detect::hasCommand(const std::string& cmd) {
    // 通过 ExecEngine::exists，不走 shell（v1 同）
    return ExecEngine::exists(cmd);
}

bool Detect::networkReachable() {
    return ExecEngine::runPing("8.8.8.8", 1, 3);
}

bool Detect::isRoot() {
    return ::getuid() == 0;
}

bool Detect::isLinux() {
    return os_ != HostOS::FreeBSD  && os_ != HostOS::OpenBSD &&
           os_ != HostOS::NetBSD   && os_ != HostOS::DragonFly &&
           os_ != HostOS::MacOS    && os_ != HostOS::Unknown;
}

bool Detect::isBSD() {
    return os_ == HostOS::FreeBSD  || os_ == HostOS::OpenBSD ||
           os_ == HostOS::NetBSD   || os_ == HostOS::DragonFly;
}

bool Detect::isDarwin() { return os_ == HostOS::MacOS; }

std::string Detect::defaultBackend() {
    // OS 映射
    switch (os_) {
        case HostOS::ArchLinux:  return "pacman";
        case HostOS::Debian:
        case HostOS::Ubuntu:     return "apt";
        case HostOS::Fedora:     return "dnf";
        case HostOS::OpenSUSE:   return "zypper";
        case HostOS::Gentoo:     return "portage";
        case HostOS::FreeBSD:    return "pkg";
        case HostOS::MacOS:      return "brew";
        default: break;
    }
    // 兜底：检测命令存在性
    if (ExecEngine::exists("apt"))    return "apt";
    if (ExecEngine::exists("pacman")) return "pacman";
    if (ExecEngine::exists("dnf"))    return "dnf";
    if (ExecEngine::exists("zypper")) return "zypper";
    if (ExecEngine::exists("brew"))   return "brew";
    return "";
}

std::vector<std::string> Detect::availableBackends() {
    std::vector<std::string> result;
    // 系统 native
    if (ExecEngine::exists("apt"))     result.push_back("apt");
    if (ExecEngine::exists("pacman"))  result.push_back("pacman");
    if (ExecEngine::exists("dnf"))     result.push_back("dnf");
    if (ExecEngine::exists("zypper"))  result.push_back("zypper");
    if (ExecEngine::exists("emerge"))  result.push_back("portage");
    if (ExecEngine::exists("pkg"))     result.push_back("pkg");
    if (ExecEngine::exists("brew"))    result.push_back("brew");
    // 通用
    if (ExecEngine::exists("flatpak")) result.push_back("flatpak");
    if (ExecEngine::exists("snap"))    result.push_back("snap");
    if (ExecEngine::exists("nix"))     result.push_back("nix");
    if (ExecEngine::exists("npm"))     result.push_back("npm");
    if (ExecEngine::exists("pip3"))    result.push_back("pip");
    return result;
}

} // namespace SSPM
