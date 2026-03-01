// ═══════════════════════════════════════════════════════════
//  layer/backends/brew_backend.cpp — Homebrew / Linuxbrew
//  PATH 检测逻辑参考 v1.9 brew.sh 的多路径策略
// ═══════════════════════════════════════════════════════════
#include "brew_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace SSPM {
namespace fs = std::filesystem;

// ── brew 路径检测（v1.9 brew.sh 逻辑移植）────────────────
// 优先级（从 v1 brew.sh 迁移）：
//   1. /opt/homebrew/bin/brew   (macOS Apple Silicon)
//   2. /usr/local/bin/brew      (macOS Intel)
//   3. /home/linuxbrew/.linuxbrew/bin/brew  (Linuxbrew 标准位置)
//   4. ~/.linuxbrew/bin/brew    (Linuxbrew 用户安装)
//   5. PATH 里的 brew
std::string BrewBackend::findBrew() {
    static const char* kKnownPaths[] = {
        "/opt/homebrew/bin/brew",
        "/usr/local/bin/brew",
        "/home/linuxbrew/.linuxbrew/bin/brew",
        nullptr
    };
    for (auto p = kKnownPaths; *p; ++p) {
        if (access(*p, X_OK) == 0) return *p;
    }
    // 用户目录的 Linuxbrew
    const char* home = getenv("HOME");
    if (home) {
        std::string user_brew = std::string(home) + "/.linuxbrew/bin/brew";
        if (access(user_brew.c_str(), X_OK) == 0) return user_brew;
    }
    // PATH 兜底
    if (ExecEngine::exists("brew")) return "brew";
    return "";
}

bool BrewBackend::available() const {
    return !findBrew().empty();
}

std::string BrewBackend::brewPrefix() {
    std::string brew = findBrew();
    if (brew.empty()) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    auto r = ExecEngine::capture(brew, {"--prefix"}, opts);
    if (!r.ok()) return "";
    auto out = r.stdout_out;
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

// Linuxbrew PATH 自动修复（v1.9 brew.sh path-fix 逻辑）
bool BrewBackend::fixPath() {
    std::string prefix = brewPrefix();
    if (prefix.empty()) return false;

    std::string binPath = prefix + "/bin";
    // 检查当前 PATH 是否已包含
    const char* path = getenv("PATH");
    if (path && std::string(path).find(binPath) != std::string::npos) {
        Logger::ok("brew PATH 已正确配置: " + binPath);
        return true;
    }

    Logger::warn("brew 不在 PATH 中，尝试修复...");

    // 写入 ~/.bashrc 和 ~/.zshrc（如果存在）
    const char* home = getenv("HOME");
    if (!home) return false;

    std::string line = "\nexport PATH=\"" + binPath + ":$PATH\"\n";
    auto appendTo = [&](const std::string& file) {
        if (fs::exists(file)) {
            std::ofstream f(file, std::ios::app);
            if (f) {
                f << "# Added by sky (brew PATH fix)\n" << line;
                Logger::ok("已写入 " + file);
            }
        }
    };

    appendTo(std::string(home) + "/.bashrc");
    appendTo(std::string(home) + "/.zshrc");
    appendTo(std::string(home) + "/.config/fish/config.fish");

    Logger::info("请重新打开终端或运行: export PATH=\"" + binPath + ":$PATH\"");
    return true;
}

BackendResult BrewBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[brew] 包名校验失败: " + v.reason};

    std::string brew = findBrew();
    if (brew.empty()) return {1, "brew not found"};

    Logger::step("brew install " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"install"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run(brew, args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("brew 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult BrewBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[brew] 包名校验失败: " + v.reason};

    std::string brew = findBrew();
    if (brew.empty()) return {1, "brew not found"};

    Logger::step("brew uninstall " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"uninstall"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run(brew, args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("brew uninstall 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult BrewBackend::upgrade() {
    std::string brew = findBrew();
    if (brew.empty()) return {1, "brew not found"};

    Logger::step("brew update && brew upgrade");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run(brew, {"update"}, opts);
    if (rc != 0) Logger::warn("brew update 失败，尝试继续...");

    rc = ExecEngine::run(brew, {"upgrade"}, opts);
    if (rc == 0) Logger::ok("系统更新完成");
    else Logger::error("brew upgrade 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult BrewBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    std::string brew = findBrew();
    if (brew.empty()) return {1, "brew not found"};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture(brew, {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string BrewBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    std::string brew = findBrew();
    if (brew.empty()) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture(brew, {"list", "--versions", "--", pkg}, opts);
    if (!r.ok() || r.stdout_out.empty()) return "";
    // 输出格式: "pkg 1.2.3 1.1.0\n"（可能多版本，取最后一个）
    auto& out = r.stdout_out;
    auto sp = out.rfind(' ');
    auto nl = out.rfind('\n');
    if (sp == std::string::npos) return "";
    std::string ver = out.substr(sp + 1);
    if (nl != std::string::npos && nl > sp) ver = out.substr(sp + 1, nl - sp - 1);
    return ver;
}

} // namespace SSPM
