// ═══════════════════════════════════════════════════════════
//  layer/backends/nix_backend.cpp — Nix 包管理器
//  PATH 修复逻辑参考 v1.9 nix.sh：
//    ~/.nix-profile/bin 追加到 PATH（不替换系统路径）
//    写入 ~/.bashrc / ~/.zshrc / ~/.config/fish/config.fish
// ═══════════════════════════════════════════════════════════
#include "nix_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace SSPM {
namespace fs = std::filesystem;

// ── Nix 路径检测（v1.9 nix.sh 逻辑移植）─────────────────
std::string NixBackend::nixProfileBin() {
    // 优先级（参考 v1.9）：
    //   1. ~/.nix-profile/bin          单用户安装
    //   2. /nix/var/nix/profiles/default/bin  多用户
    //   3. /run/current-system/sw/bin  NixOS
    static const char* kPaths[] = {
        nullptr,  // 占位，下面动态填充 HOME
        "/nix/var/nix/profiles/default/bin",
        "/run/current-system/sw/bin",
        nullptr
    };

    const char* home = getenv("HOME");
    std::string homeNix;
    if (home) {
        homeNix = std::string(home) + "/.nix-profile/bin";
        if (access(homeNix.c_str(), X_OK) == 0) return homeNix;
    }

    for (int i = 1; kPaths[i]; ++i) {
        if (access(kPaths[i], X_OK) == 0) return kPaths[i];
    }
    return "";
}

bool NixBackend::daemonRunning() {
    // nix-daemon socket: /nix/var/nix/daemon-socket/socket
    return access("/nix/var/nix/daemon-socket/socket", F_OK) == 0;
}

bool NixBackend::available() const {
    // 检查 nix 命令可用（可能在 nix-profile/bin 里，不一定在系统 PATH）
    std::string profileBin = nixProfileBin();
    if (!profileBin.empty()) {
        std::string nix = profileBin + "/nix";
        if (access(nix.c_str(), X_OK) == 0) return true;
    }
    return ExecEngine::exists("nix");
}

// PATH 自动修复（v1.9 nix.sh path-fix 逻辑）
bool NixBackend::fixPath() {
    std::string profileBin = nixProfileBin();
    if (profileBin.empty()) {
        Logger::warn("未找到 nix-profile bin 目录");
        return false;
    }

    // 检查当前 PATH
    const char* path = getenv("PATH");
    if (path && std::string(path).find(profileBin) != std::string::npos) {
        Logger::ok("nix PATH 已正确配置: " + profileBin);
        return true;
    }

    Logger::warn("nix 不在 PATH 中，尝试修复...");

    const char* home = getenv("HOME");
    if (!home) return false;

    // 取自 v1.9 nix.sh 的写入逻辑，同时处理 bash/zsh/fish
    std::string bashLine  = "\n# Added by sky (nix PATH fix)\nexport PATH=\"" + profileBin + ":$PATH\"\n";
    std::string fishLine  = "\n# Added by sky (nix PATH fix)\nset -gx PATH " + profileBin + " $PATH\n";

    auto appendTo = [&](const std::string& file, const std::string& content) {
        if (fs::exists(file)) {
            std::ofstream f(file, std::ios::app);
            if (f) f << content, Logger::ok("已写入 " + file);
        }
    };

    appendTo(std::string(home) + "/.bashrc",  bashLine);
    appendTo(std::string(home) + "/.zshrc",   bashLine);
    appendTo(std::string(home) + "/.config/fish/config.fish", fishLine);

    // nix 还需要 source profile（v1.9 nix.sh 也做了这步）
    std::string profileScript = std::string(home) + "/.nix-profile/etc/profile.d/nix.sh";
    if (fs::exists(profileScript)) {
        Logger::info("提示: 运行 'source " + profileScript + "' 立即生效");
    } else {
        Logger::info("请重新打开终端或运行: export PATH=\"" + profileBin + ":$PATH\"");
    }
    return true;
}

BackendResult NixBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[nix] 包名校验失败: " + v.reason};

    Logger::step("nix-env -iA nixpkgs." + pkgs[0] + (pkgs.size() > 1 ? " ..." : ""));

    // nix-env -iA nixpkgs.<pkg>（推荐用属性路径安装）
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        // 先尝试属性路径，失败则用包名
        int rc = ExecEngine::run("nix-env",
            {"-iA", "nixpkgs." + pkg}, opts);
        if (rc != 0) {
            Logger::warn("属性路径失败，尝试包名安装...");
            rc = ExecEngine::run("nix-env", {"-i", "--", pkg}, opts);
        }
        if (rc == 0) Logger::ok("已安装: " + pkg);
        else {
            Logger::error("nix-env 安装 " + pkg + " 失败 exit=" + std::to_string(rc));
            lastRc = rc;
        }
    }
    return {lastRc, ""};
}

BackendResult NixBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[nix] 包名校验失败: " + v.reason};

    Logger::step("nix-env -e " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    std::vector<std::string> args = {"-e"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    int rc = ExecEngine::run("nix-env", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("nix-env -e 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult NixBackend::upgrade() {
    Logger::step("nix-channel --update && nix-env -u");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("nix-channel", {"--update"}, opts);
    if (rc != 0) Logger::warn("nix-channel --update 失败，尝试继续...");

    rc = ExecEngine::run("nix-env", {"-u"}, opts);
    if (rc == 0) Logger::ok("Nix 环境更新完成");
    else Logger::error("nix-env -u 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult NixBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    // nix search nixpkgs <query>（nix 3.x+）
    auto r = ExecEngine::capture("nix", {"search", "nixpkgs", "--", query}, opts);
    if (!r.ok()) {
        // 回退到 nix-env --query（旧版）
        r = ExecEngine::capture("nix-env", {"-qaP", "--", query}, opts);
    }
    return {r.exitCode, r.stdout_out};
}

std::string NixBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    // nix-env -q <pkg> 输出 "pkg-1.2.3"
    auto r = ExecEngine::capture("nix-env", {"-q", "--", pkg}, opts);
    if (!r.ok() || r.stdout_out.empty()) return "";
    auto& out = r.stdout_out;
    auto dash = out.rfind('-');
    auto nl   = out.find('\n');
    if (dash == std::string::npos) return "";
    std::string ver = out.substr(dash + 1);
    if (nl != std::string::npos) ver = ver.substr(0, nl - (dash + 1));
    return ver;
}

} // namespace SSPM
