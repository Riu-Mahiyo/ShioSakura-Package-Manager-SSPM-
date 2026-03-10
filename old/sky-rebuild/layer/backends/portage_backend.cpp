// layer/backends/portage_backend.cpp — Portage Backend (Gentoo)
#include "portage_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool PortageBackend::available() const {
    return ExecEngine::exists("emerge");
}

BackendResult PortageBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[portage] 包名校验失败: " + v.reason};

    Logger::step("emerge --ask=n " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"--ask=n", "--quiet-build"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 3600;  // Gentoo 编译可能很慢

    int rc = ExecEngine::run("emerge", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("emerge 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PortageBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[portage] 包名校验失败: " + v.reason};

    Logger::step("emerge --unmerge " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"--ask=n", "--unmerge"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("emerge", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("emerge unmerge 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PortageBackend::upgrade() {
    Logger::step("emerge --sync && emerge -uDN @world");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 7200;

    // sync portage tree
    int rc = ExecEngine::run("emerge", {"--sync", "--quiet"}, opts);
    if (rc != 0) {
        Logger::warn("emerge --sync 失败，尝试继续更新...");
    }

    rc = ExecEngine::run("emerge",
        {"--ask=n", "--update", "--deep", "--newuse", "@world"}, opts);
    if (rc == 0) Logger::ok("系统更新完成");
    else Logger::error("emerge world 更新失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PortageBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("emerge", {"--search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string PortageBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    // equery list <pkg> 输出 "category/pkg-version"
    auto r = ExecEngine::capture("equery", {"list", "--", pkg}, opts);
    if (!r.ok() || r.stdout_out.empty()) return "";
    // 取第一行，提取版本
    auto& out = r.stdout_out;
    auto nl = out.find('\n');
    std::string line = (nl == std::string::npos) ? out : out.substr(0, nl);
    // 格式: [IP-] [  ] category/pkg-1.2.3:0
    auto dash = line.rfind('-');
    auto colon = line.rfind(':');
    if (dash == std::string::npos) return "";
    return line.substr(dash + 1, (colon != std::string::npos) ? colon - dash - 1 : std::string::npos);
}

} // namespace SSPM
