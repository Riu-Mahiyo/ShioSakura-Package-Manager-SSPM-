// layer/backends/pacman_backend.cpp — Pacman Backend (Arch Linux)
#include "pacman_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool PacmanBackend::available() const {
    return ExecEngine::exists("pacman");
}

BackendResult PacmanBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[pacman] 包名校验失败: " + v.reason};

    Logger::step("pacman -S --noconfirm " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"-S", "--noconfirm"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("pacman", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("pacman 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PacmanBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[pacman] 包名校验失败: " + v.reason};

    Logger::step("pacman -R --noconfirm " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"-R", "--noconfirm"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("pacman", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("pacman remove 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PacmanBackend::upgrade() {
    Logger::step("pacman -Syu --noconfirm");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("pacman", {"-Syu", "--noconfirm"}, opts);
    if (rc == 0) Logger::ok("系统更新完成");
    else Logger::error("pacman -Syu 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PacmanBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("pacman", {"-Ss", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string PacmanBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture(
        "pacman", {"-Q", "--", pkg}, opts);
    if (!r.ok()) return "";
    // 输出格式: "pkg 1.2.3-1\n"
    auto out = r.stdout_out;
    auto sp  = out.find(' ');
    if (sp == std::string::npos) return "";
    auto ver = out.substr(sp + 1);
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
