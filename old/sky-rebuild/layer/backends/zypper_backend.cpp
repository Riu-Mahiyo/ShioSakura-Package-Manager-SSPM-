// layer/backends/zypper_backend.cpp — Zypper Backend (openSUSE)
#include "zypper_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool ZypperBackend::available() const {
    return ExecEngine::exists("zypper");
}

BackendResult ZypperBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[zypper] 包名校验失败: " + v.reason};

    Logger::step("zypper install -y " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"--non-interactive", "install"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("zypper", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("zypper 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult ZypperBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[zypper] 包名校验失败: " + v.reason};

    Logger::step("zypper remove -y " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"--non-interactive", "remove"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("zypper", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("zypper remove 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult ZypperBackend::upgrade() {
    Logger::step("zypper refresh && zypper update -y");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("zypper", {"--non-interactive", "refresh"}, opts);
    if (rc != 0) return {rc, "zypper refresh 失败"};

    rc = ExecEngine::run("zypper", {"--non-interactive", "update"}, opts);
    if (rc == 0) Logger::ok("系统更新完成");
    else Logger::error("zypper update 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult ZypperBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("zypper", {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string ZypperBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture(
        "rpm", {"-q", "--queryformat", "%{VERSION}", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
