// layer/backends/gem_backend.cpp — Ruby gem 包管理器
#include "gem_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

#include <sstream>
#include <unistd.h>

namespace SSPM {

bool GemBackend::available() const {
    return ExecEngine::exists("gem");
}

BackendResult GemBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[gem] 包名校验失败: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    // 非 root 使用 --user-install
    bool isRoot = (getuid() == 0);
    std::vector<std::string> baseArgs = {"install"};
    if (!isRoot) baseArgs.push_back("--user-install");

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        std::vector<std::string> args = baseArgs;
        args.push_back("--");
        args.push_back(pkg);

        Logger::step("gem install " + pkg);
        int rc = ExecEngine::run("gem", args, opts);
        if (rc == 0) Logger::ok("已安装: " + pkg);
        else { Logger::error("gem install " + pkg + " 失败"); lastRc = rc; }
    }
    return {lastRc, ""};
}

BackendResult GemBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[gem] 包名校验失败: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 60;

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        int rc = ExecEngine::run("gem", {"uninstall", "-x", "--", pkg}, opts);
        if (rc == 0) Logger::ok("已卸载: " + pkg);
        else { Logger::error("gem uninstall " + pkg + " 失败"); lastRc = rc; }
    }
    return {lastRc, ""};
}

BackendResult GemBackend::upgrade() {
    Logger::step("gem update");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("gem", {"update"}, opts);
    if (rc == 0) Logger::ok("所有 gem 已更新");
    return {rc, ""};
}

BackendResult GemBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法"};

    Logger::step("gem search " + query);
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 20;

    int rc = ExecEngine::run("gem", {"search", "--", query}, opts);
    return {rc, ""};
}

std::string GemBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture("gem", {"list", "--", "^" + pkg + "$"}, opts);
    if (!r.ok() || r.stdout_out.empty()) return "";

    // 格式：rails (7.0.0, 6.1.0)
    std::string out = r.stdout_out;
    auto paren = out.find(" (");
    auto close = out.find(')');
    if (paren != std::string::npos && close != std::string::npos) {
        std::string versions = out.substr(paren + 2, close - paren - 2);
        // 取第一个（最新已安装）
        auto comma = versions.find(", ");
        return comma != std::string::npos ? versions.substr(0, comma) : versions;
    }
    return "";
}

} // namespace SSPM
