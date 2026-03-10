// layer/backends/pkg_backend.cpp — FreeBSD pkg
#include "pkg_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool PkgBackend::available() const {
    return ExecEngine::exists("pkg");
}

BackendResult PkgBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[pkg] 包名校验失败: " + v.reason};

    Logger::step("pkg install -y " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"install", "-y"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("pkg", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("pkg 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PkgBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[pkg] 包名校验失败: " + v.reason};

    Logger::step("pkg remove -y " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"remove", "-y"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("pkg", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("pkg remove 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PkgBackend::upgrade() {
    Logger::step("pkg upgrade -y");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("pkg", {"upgrade", "-y"}, opts);
    if (rc == 0) Logger::ok("系统更新完成");
    else Logger::error("pkg upgrade 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PkgBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("pkg", {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string PkgBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    // pkg info -e <pkg> + pkg query "%v" <pkg>
    auto r = ExecEngine::capture("pkg", {"query", "%v", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
