// layer/backends/npm_backend.cpp — npm global packages
#include "npm_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool NpmBackend::available() const {
    return ExecEngine::exists("npm");
}

BackendResult NpmBackend::install(const std::vector<std::string>& pkgs) {
    // npm 包名允许 @ 和 / （如 @angular/cli）
    for (auto& p : pkgs) {
        auto v = InputValidator::safeString(p, 128);
        if (!v) return {1, "[npm] 包名非法: " + v.reason};
    }

    Logger::step("npm install -g " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"install", "-g"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("npm", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("npm 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult NpmBackend::remove(const std::vector<std::string>& pkgs) {
    for (auto& p : pkgs) {
        auto v = InputValidator::safeString(p, 128);
        if (!v) return {1, "[npm] 包名非法: " + v.reason};
    }

    Logger::step("npm uninstall -g " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"uninstall", "-g"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("npm", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("npm uninstall 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult NpmBackend::upgrade() {
    Logger::step("npm update -g");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("npm", {"update", "-g"}, opts);
    if (rc == 0) Logger::ok("npm 全局包更新完成");
    else Logger::error("npm update -g 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult NpmBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("npm", {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string NpmBackend::installedVersion(const std::string& pkg) {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    // npm list -g <pkg> --depth=0
    auto r = ExecEngine::capture("npm",
        {"list", "-g", "--depth=0", "--", pkg}, opts);
    if (!r.ok()) return "";
    // 输出: "└── pkg@1.2.3"
    auto& out = r.stdout_out;
    auto at = out.rfind('@');
    auto nl = out.find('\n');
    if (at == std::string::npos) return "";
    std::string ver = out.substr(at + 1);
    if (nl != std::string::npos && nl > at) ver = ver.substr(0, nl - at - 1);
    // trim
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r' || ver.back() == ' '))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
