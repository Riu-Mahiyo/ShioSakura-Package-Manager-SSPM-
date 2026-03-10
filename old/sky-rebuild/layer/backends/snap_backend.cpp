// layer/backends/snap_backend.cpp — Snap Backend
#include "snap_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool SnapBackend::snapdRunning() {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;
    // snap version 成功 = snapd 运行中
    auto r = ExecEngine::capture("snap", {"version"}, opts);
    return r.ok();
}

bool SnapBackend::available() const {
    if (!ExecEngine::exists("snap")) return false;
    // snap 存在但 snapd 未运行时，安装会失败
    return snapdRunning();
}

BackendResult SnapBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[snap] 包名校验失败: " + v.reason};

    Logger::step("snap install " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    // snap 一次只能安装一个包
    int lastRc = 0;
    for (auto& pkg : pkgs) {
        int rc = ExecEngine::run("snap", {"install", "--", pkg}, opts);
        if (rc != 0) {
            Logger::error("snap install " + pkg + " 失败 exit=" + std::to_string(rc));
            lastRc = rc;
        } else {
            Logger::ok("已安装: " + pkg);
        }
    }
    return {lastRc, ""};
}

BackendResult SnapBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[snap] 包名校验失败: " + v.reason};

    Logger::step("snap remove " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        int rc = ExecEngine::run("snap", {"remove", "--", pkg}, opts);
        if (rc != 0) {
            Logger::error("snap remove " + pkg + " 失败 exit=" + std::to_string(rc));
            lastRc = rc;
        } else {
            Logger::ok("已卸载: " + pkg);
        }
    }
    return {lastRc, ""};
}

BackendResult SnapBackend::upgrade() {
    Logger::step("snap refresh");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("snap", {"refresh"}, opts);
    if (rc == 0) Logger::ok("Snap 应用更新完成");
    else Logger::error("snap refresh 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult SnapBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("snap", {"find", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string SnapBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    // snap list <pkg> — 输出有 Rev 列而非版本，取 version 列
    auto r = ExecEngine::capture("snap", {"list", "--", pkg}, opts);
    if (!r.ok()) return "";
    // 格式: Name  Version  Rev  Tracking  Publisher  Notes
    //       curl  7.88.1   123  latest/stable ...
    auto& out = r.stdout_out;
    auto nl = out.find('\n');
    if (nl == std::string::npos) return "";
    auto line2 = out.substr(nl + 1);
    // 找第一个 token（name），跳过；第二个是 version
    auto sp1 = line2.find(' ');
    if (sp1 == std::string::npos) return "";
    while (sp1 < line2.size() && line2[sp1] == ' ') sp1++;
    auto sp2 = line2.find(' ', sp1);
    if (sp2 == std::string::npos) return "";
    return line2.substr(sp1, sp2 - sp1);
}

} // namespace SSPM
