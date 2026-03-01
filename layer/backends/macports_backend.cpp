// layer/backends/macports_backend.cpp — MacPorts Backend
#include "macports_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"
#include <unistd.h>

namespace SSPM {

bool MacPortsBackend::available() const {
    // port 通常在 /opt/local/bin/port
    if (ExecEngine::exists("port")) return true;
    return access("/opt/local/bin/port", X_OK) == 0;
}

static std::string portCmd() {
    if (access("/opt/local/bin/port", X_OK) == 0) return "/opt/local/bin/port";
    return "port";
}

BackendResult MacPortsBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[macports] 包名校验失败: " + v.reason};

    Logger::step("port install " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"install"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 3600;  // MacPorts 经常需要编译

    int rc = ExecEngine::run(portCmd(), args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("port install 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult MacPortsBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[macports] 包名校验失败: " + v.reason};

    Logger::step("port uninstall " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"uninstall"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run(portCmd(), args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("port uninstall 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult MacPortsBackend::upgrade() {
    Logger::step("port selfupdate && port upgrade outdated");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 3600;

    int rc = ExecEngine::run(portCmd(), {"selfupdate"}, opts);
    if (rc != 0) Logger::warn("port selfupdate 失败，继续...");

    rc = ExecEngine::run(portCmd(), {"upgrade", "outdated"}, opts);
    if (rc == 0) Logger::ok("MacPorts 更新完成");
    else Logger::error("port upgrade outdated 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult MacPortsBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture(portCmd(), {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string MacPortsBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture(portCmd(), {"info", "--version", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
