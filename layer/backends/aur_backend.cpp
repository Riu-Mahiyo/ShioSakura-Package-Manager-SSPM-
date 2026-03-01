// layer/backends/aur_backend.cpp — AUR Backend
// 参考 v1.9 aur.sh 的 helper 检测策略
// 安全：AUR 包运行 makepkg，代码可能不可信，未来 Phase 3 加签名校验
#include "aur_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"
#include <unistd.h>

#include <cstdlib>
#include <filesystem>

namespace SSPM {
namespace fs = std::filesystem;

std::string AurBackend::aurHelper() {
    // 优先级：yay > paru > makepkg（fallback）
    if (ExecEngine::exists("yay"))  return "yay";
    if (ExecEngine::exists("paru")) return "paru";
    return "";  // 无 helper，需手动 makepkg
}

bool AurBackend::available() const {
    // AUR 只在 Arch Linux 上有意义
    // 检查 pacman 存在（是 Arch 系的前提），且有 git（makepkg 需要）
    return ExecEngine::exists("pacman") && ExecEngine::exists("makepkg");
}

BackendResult AurBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[aur] 包名校验失败: " + v.reason};

    // AUR 严禁 root（makepkg 拒绝以 root 运行）
    if (getuid() == 0) {
        Logger::error("[AUR] 不能以 root 安装 AUR 包（makepkg 安全限制）");
        Logger::info("请切换到普通用户，或使用 sudo -u <user> sspm install --backend=aur <pkg>");
        return {1, "AUR 禁止 root"};
    }

    std::string helper = aurHelper();
    if (!helper.empty()) {
        Logger::step(helper + " -S --noconfirm " + [&]{
            std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

        std::vector<std::string> args = {"-S", "--noconfirm"};
        args.insert(args.end(), pkgs.begin(), pkgs.end());

        ExecEngine::Options opts;
        opts.captureOutput = false;
        opts.timeoutSec    = 600;

        int rc = ExecEngine::run(helper, args, opts);
        if (rc == 0) Logger::ok("AUR 安装成功");
        else Logger::error(helper + " 失败 exit=" + std::to_string(rc));
        return {rc, ""};
    }

    // fallback: 逐包 makepkg 安装
    Logger::warn("未找到 AUR helper（yay/paru），使用 makepkg 手动安装");
    int lastRc = 0;
    for (auto& pkg : pkgs) {
        auto r = makepkgInstall(pkg);
        if (!r.ok()) lastRc = r.exitCode;
    }
    return {lastRc, ""};
}

BackendResult AurBackend::makepkgInstall(const std::string& pkg) {
    // 需要 git 克隆 AUR 仓库
    if (!ExecEngine::exists("git")) {
        Logger::error("makepkg 安装需要 git");
        return {1, "git not found"};
    }

    const char* home = getenv("HOME");
    std::string buildDir = std::string(home ? home : "/tmp") +
                           "/.cache/sspm/aur/" + pkg;

    Logger::step("git clone https://aur.archlinux.org/" + pkg + ".git " + buildDir);

    // 清理旧目录
    try { fs::remove_all(buildDir); } catch (...) {}

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    // 克隆 AUR 仓库
    int rc = ExecEngine::run("git", {
        "clone", "--depth=1",
        "https://aur.archlinux.org/" + pkg + ".git",
        buildDir
    }, opts);

    if (rc != 0) {
        Logger::error("AUR 仓库克隆失败: " + pkg);
        return {rc, ""};
    }

    // makepkg -si --noconfirm
    opts.workDir = buildDir;
    opts.timeoutSec = 1800;  // 编译可能很慢

    rc = ExecEngine::run("makepkg", {"-si", "--noconfirm"}, opts);
    if (rc == 0) Logger::ok("已安装: " + pkg);
    else Logger::error("makepkg 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult AurBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[aur] 包名校验失败: " + v.reason};

    // AUR 包通过 pacman 卸载
    Logger::step("pacman -R --noconfirm (AUR 包通过 pacman 卸载)");

    std::vector<std::string> args = {"-R", "--noconfirm"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("pacman", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("pacman 卸载失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult AurBackend::upgrade() {
    std::string helper = aurHelper();
    if (helper.empty()) {
        Logger::warn("未找到 AUR helper，无法自动升级 AUR 包");
        Logger::info("请安装 yay: sspm install --backend=aur yay");
        return {0, ""};
    }

    Logger::step(helper + " -Syu --noconfirm");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run(helper, {"-Syu", "--noconfirm"}, opts);
    if (rc == 0) Logger::ok("AUR 更新完成");
    else Logger::error(helper + " 更新失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult AurBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    std::string helper = aurHelper();
    if (!helper.empty()) {
        ExecEngine::Options opts;
        opts.captureOutput = true;
        opts.timeoutSec    = 30;
        auto r = ExecEngine::capture(helper, {"-Ss", "--aur", "--", query}, opts);
        return {r.exitCode, r.stdout_out};
    }

    // 无 helper：用 pacman -Ss 搜索 official + 提示
    Logger::warn("AUR 搜索需要 yay/paru，当前只显示官方仓库结果");
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;
    auto r = ExecEngine::capture("pacman", {"-Ss", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string AurBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    // AUR 包安装后由 pacman 管理
    auto r = ExecEngine::capture("pacman", {"-Q", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto& out = r.stdout_out;
    auto sp = out.find(' ');
    if (sp == std::string::npos) return "";
    auto ver = out.substr(sp + 1);
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
