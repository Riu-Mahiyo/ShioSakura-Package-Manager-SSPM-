// ═══════════════════════════════════════════════════════════
//  core/backend_setup.cpp — Backend 安装向导实现
// ═══════════════════════════════════════════════════════════
#include "backend_setup.h"
#include "exec_engine.h"
#include "logger.h"
#include "mirror.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

namespace SSPM {
namespace fs = std::filesystem;

// ── Backend 信息数据库 ────────────────────────────────────
std::vector<BackendInstallInfo> BackendSetup::buildDatabase() {
    return {
        {"apt",      "Debian/Ubuntu 原生包管理器",
         "apt",       "系统自带",                false, true,  false, false, ""},
        {"dnf",      "Fedora/RHEL 包管理器",
         "dnf",       "系统自带",                false, true,  false, false, ""},
        {"pacman",   "Arch Linux 包管理器",
         "pacman",    "系统自带",                false, true,  false, false, ""},
        {"zypper",   "openSUSE 包管理器",
         "zypper",    "系统自带",                false, true,  false, false, ""},
        {"portage",  "Gentoo 包管理器",
         "emerge",    "系统自带",                false, true,  false, false, ""},

        {"flatpak",  "跨发行版沙箱包格式",
         "flatpak",   "apt/dnf install flatpak", true,  true,  false, false, ""},
        {"snap",     "Canonical 通用包格式",
         "snap",      "apt install snapd",       true,  true,  false, false, ""},
        {"nix",      "函数式包管理器",
         "nix-env",   "sh <(curl -L https://nixos.org/nix/install)", false, true, true, false, ""},

        {"brew",     "macOS/Linux Homebrew",
         "brew",      "/bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"",
         false, true, true, false, "12.0"},  // brew 要求 macOS >= 12

        {"macports", "MacPorts (支持老版本 macOS)",
         "port",      "pkg 安装器（支持老版本 macOS）",
         false, false, true, false, ""},  // 无最低版本限制

        {"pkg",      "FreeBSD 包管理器",
         "pkg",       "系统自带",                false, false, false, true, ""},

        {"npm",      "Node.js 包管理器",
         "npm",       "apt/brew install nodejs",  false, true, true, true, ""},
        {"pip",      "Python 包管理器",
         "pip3",      "apt/brew install python3", false, true, true, true, ""},
        {"cargo",    "Rust 包管理器",
         "cargo",     "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh",
         false, true, true, true, ""},
        {"gem",      "Ruby 包管理器",
         "gem",       "apt/brew install ruby",    false, true, true, true, ""},
    };
}

const std::vector<BackendInstallInfo>& BackendSetup::db() {
    static auto instance = buildDatabase();
    return instance;
}

const BackendInstallInfo* BackendSetup::getInfo(const std::string& backend) {
    for (auto& info : db()) {
        if (info.name == backend) return &info;
    }
    return nullptr;
}

// ── 安装检测 ──────────────────────────────────────────────
bool BackendSetup::isInstalled(const std::string& backend) {
    auto info = getInfo(backend);
    if (!info) return ExecEngine::exists(backend);
    return ExecEngine::exists(info->checkCmd);
}

// ── 交互确认 ──────────────────────────────────────────────
bool BackendSetup::askUser(const std::string& prompt) {
    std::cout << "\033[1;33m[?] " << prompt << " [y/N] \033[0m" << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) return false;
    if (line.empty()) return false;
    return (line[0] == 'y' || line[0] == 'Y');
}

// ── macOS 版本检测 ────────────────────────────────────────
std::string BackendSetup::getMacOSVersion() {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;
    auto r = ExecEngine::capture("sw_vers", {"-productVersion"}, opts);
    if (!r.ok()) return "";
    std::string v = r.stdout_out;
    while (!v.empty() && (v.back() == '\n' || v.back() == '\r')) v.pop_back();
    return v;
}

bool BackendSetup::isMacOSVersionOldEnoughForBrewBlock() {
    std::string ver = getMacOSVersion();
    if (ver.empty()) return false;
    // 解析主版本号
    try {
        int major = std::stoi(ver.substr(0, ver.find('.')));
        return major < 12;  // macOS < 12 (Monterey) brew 支持有限
    } catch (...) {
        return false;
    }
}

bool BackendSetup::shouldSuggestMacPortsForOldMacOS() {
    return isMacOSVersionOldEnoughForBrewBlock();
}

// ── MacPorts 老版本 macOS 安装 ────────────────────────────
bool BackendSetup::installMacPortsForOldMacOS() {
    std::string ver = getMacOSVersion();
    Logger::info("检测到 macOS " + ver + "（老版本）");
    Logger::info("MacPorts 支持此版本，正在提供安装引导...");

    // MacPorts 提供针对老版本的 .pkg 安装器
    // 判断架构
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;
    auto r = ExecEngine::capture("uname", {"-m"}, opts);
    std::string arch = r.ok() ? r.stdout_out : "x86_64";
    while (!arch.empty() && (arch.back() == '\n' || arch.back() == '\r')) arch.pop_back();

    // MacPorts 下载页面（按 macOS 版本）
    std::string pkgUrl = "https://www.macports.org/install.php";
    Logger::info("请访问: " + pkgUrl);
    Logger::info("下载对应 macOS " + ver + " 版本的 MacPorts .pkg 安装器");
    Logger::info("安装后运行: sudo port selfupdate");

    // 尝试用浏览器打开
    ExecEngine::Options openOpts;
    openOpts.captureOutput = false;
    openOpts.timeoutSec    = 5;
    ExecEngine::run("open", {pkgUrl}, openOpts);

    return false;  // 需要用户手动完成安装
}

// ── 具体 backend 安装实现 ─────────────────────────────────
bool BackendSetup::doInstallBrew() {
#ifdef __APPLE__
    // macOS - 检查系统版本
    if (isMacOSVersionOldEnoughForBrewBlock()) {
        std::string ver = getMacOSVersion();
        Logger::warn("您的 macOS 版本 (" + ver + ") 对 Homebrew 的支持有限");
        Logger::info("建议使用 MacPorts，它完整支持老版本 macOS");
        if (askUser("改为安装 MacPorts？")) {
            return installMacPortsForOldMacOS();
        }
        return false;
    }
    Logger::info("正在安装 Homebrew...");
    Logger::info("这将从 GitHub 下载安装脚本，请确保网络通畅");
#else
    Logger::info("正在安装 Linuxbrew（Homebrew for Linux）...");
    Logger::info("安装后需要将 ~/.linuxbrew/bin 加入 PATH");
#endif
    Logger::info("官方安装命令：");
    Logger::info("  /bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"");
    Logger::info("请手动执行上述命令，或设置 SSPM_REGION 后重试");
    return false;
}

bool BackendSetup::doInstallFlatpak() {
    Logger::step("安装 Flatpak...");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = -1;
    if (ExecEngine::exists("apt")) {
        rc = ExecEngine::run("apt", {"-y", "install", "flatpak"}, opts);
    } else if (ExecEngine::exists("dnf")) {
        rc = ExecEngine::run("dnf", {"-y", "install", "flatpak"}, opts);
    } else if (ExecEngine::exists("pacman")) {
        rc = ExecEngine::run("pacman", {"-S", "--noconfirm", "flatpak"}, opts);
    }

    if (rc != 0) {
        Logger::error("Flatpak 安装失败");
        return false;
    }

    // 添加 Flathub
    Logger::step("配置 Flathub 仓库...");
    auto region = MirrorManager::instance().detectRegion();
    std::string flathubUrl = (region == "CN")
        ? "https://mirror.sjtu.edu.cn/flathub"  // 上海交大镜像
        : "https://flathub.org/repo/flathub.flatpakrepo";

    ExecEngine::run("flatpak", {"remote-add", "--if-not-exists", "flathub", flathubUrl}, opts);
    Logger::ok("Flatpak 安装完成，Flathub 已配置");
    return true;
}

bool BackendSetup::doInstallSnap() {
    Logger::step("安装 snapd...");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = -1;
    if (ExecEngine::exists("apt")) {
        rc = ExecEngine::run("apt", {"-y", "install", "snapd"}, opts);
    }

    if (rc != 0) { Logger::error("snapd 安装失败"); return false; }

    // 启动 snapd
    ExecEngine::run("systemctl", {"enable", "--now", "snapd"}, opts);
    Logger::ok("snapd 安装并启动完成");
    return true;
}

bool BackendSetup::doInstallNix() {
    Logger::step("安装 Nix 包管理器...");
    Logger::info("Nix 安装脚本需要从 nixos.org 下载，如在中国境内访问可能较慢");

    auto region = MirrorManager::instance().detectRegion();
    if (region == "CN") {
        Logger::info("检测到中国区域，建议安装完成后配置清华镜像：");
        Logger::info("  echo 'substituters = https://mirrors.tuna.tsinghua.edu.cn/nix-channels/store https://cache.nixos.org/' >> ~/.config/nix/nix.conf");
    }

    Logger::info("官方安装命令（多用户模式）：");
    Logger::info("  sh <(curl -L https://nixos.org/nix/install) --daemon");
    return false;  // 需要用户手动完成
}

bool BackendSetup::doInstallCargo() {
    Logger::step("安装 Rust 工具链（包含 cargo）...");
    auto region = MirrorManager::instance().detectRegion();

    if (region == "CN") {
        Logger::info("检测到中国区域，建议先设置镜像：");
        Logger::info("  export RUSTUP_DIST_SERVER=https://mirrors.tuna.tsinghua.edu.cn/rustup");
    }

    Logger::info("官方安装命令：");
    Logger::info("  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh");
    return false;
}

bool BackendSetup::doInstallGem() {
    Logger::step("检查 Ruby...");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = -1;
    if (ExecEngine::exists("apt")) {
        rc = ExecEngine::run("apt", {"-y", "install", "ruby-full"}, opts);
    } else if (ExecEngine::exists("brew")) {
        rc = ExecEngine::run("brew", {"install", "ruby"}, opts);
    }

    if (rc != 0) { Logger::error("Ruby 安装失败"); return false; }

    // 设置 gem 镜像（中国区域）
    auto region = MirrorManager::instance().detectRegion();
    if (region == "CN") {
        ExecEngine::run("gem", {"sources", "--add",
            "https://gems.ruby-china.com/",
            "--remove", "https://rubygems.org/"}, opts);
        Logger::ok("gem 镜像已设置为 gems.ruby-china.com");
    }
    return true;
}

// ── 主要公开接口 ──────────────────────────────────────────
bool BackendSetup::install(const std::string& backend) {
    if (backend == "brew" || backend == "homebrew") return doInstallBrew();
    if (backend == "flatpak")  return doInstallFlatpak();
    if (backend == "snap")     return doInstallSnap();
    if (backend == "nix")      return doInstallNix();
    if (backend == "cargo")    return doInstallCargo();
    if (backend == "gem")      return doInstallGem();
    if (backend == "macports") return doInstallMacPorts();

    auto info = getInfo(backend);
    if (info) {
        Logger::info("安装 " + backend + " 的方法：");
        Logger::info("  " + info->installCmd);
    } else {
        Logger::warn("未知 backend: " + backend);
    }
    return false;
}

bool BackendSetup::doInstallMacPorts() {
    if (shouldSuggestMacPortsForOldMacOS()) {
        return installMacPortsForOldMacOS();
    }
    Logger::info("请访问 https://www.macports.org/install.php 下载安装器");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec = 3;
    ExecEngine::run("open", {"https://www.macports.org/install.php"}, opts);
    return false;
}

bool BackendSetup::ensureAvailable(const std::string& backend, bool interactive) {
    if (isInstalled(backend)) return true;

    auto info = getInfo(backend);
    std::string desc = info ? info->description : backend;

    Logger::warn("Backend '" + backend + "' 未安装");
    if (info && !info->installCmd.empty()) {
        Logger::info("安装方法: " + info->installCmd);
    }

    if (!interactive) return false;

    if (!askUser("是否现在安装 " + backend + "（" + desc + "）？")) {
        return false;
    }

    bool ok = install(backend);
    if (ok) {
        // 自动配置镜像
        MirrorManager::instance().applyMirror(backend);
        Logger::ok(backend + " 安装并配置完成");
    }
    return ok && isInstalled(backend);
}

void BackendSetup::checkAndPromptAll(const std::vector<std::string>& backends) {
    std::cout << "\033[1m\n正在检查 Backend 安装状态...\033[0m\n\n";

    for (auto& b : backends) {
        bool installed = isInstalled(b);
        auto info = getInfo(b);
        std::string desc = info ? info->description : b;

        std::cout << "  " << (installed ? "\033[1;32m✓\033[0m" : "\033[0;90m✗\033[0m")
                  << "  " << std::left << b;
        if (!installed && info) {
            std::cout << "\033[0;90m  (" << desc << ")\033[0m";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

} // namespace SSPM
