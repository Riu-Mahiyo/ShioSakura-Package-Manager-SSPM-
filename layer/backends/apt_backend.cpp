// ═══════════════════════════════════════════════════════════
//  layer/backends/apt_backend.cpp — APT Backend
//
//  设计原则：
//    ✗ 不用 system("apt install " + pkg)  ← shell 注入风险
//    ✓ 用 ExecEngine::run("apt", {...})   ← 参数独立，安全
//    ✓ 包名先经 InputValidator::pkgList() 验证
// ═══════════════════════════════════════════════════════════
#include "apt_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool AptBackend::available() const {
    return ExecEngine::exists("apt");
}

BackendResult AptBackend::install(const std::vector<std::string>& pkgs) {
    // 验证所有包名
    auto v = InputValidator::pkgList(pkgs);
    if (!v) {
        return {1, "[apt] 包名校验失败: " + v.reason};
    }

    Logger::step("apt install " + [&]{
        std::string s;
        for (auto& p : pkgs) s += p + " ";
        return s;
    }());

    // 构建参数（-y 非交互，DEBIAN_FRONTEND=noninteractive 禁止 dpkg 问问题）
    std::vector<std::string> args = {"install", "-y"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;  // 透传输出到终端
    opts.timeoutSec    = 300;    // 5 分钟超时
    opts.extraEnv["DEBIAN_FRONTEND"] = "noninteractive";

    int rc = ExecEngine::run("apt", args, opts);
    if (rc == 0) {
        Logger::ok("安装成功");
    } else {
        Logger::error("apt 返回 exit=" + std::to_string(rc));
    }
    return {rc, ""};
}

BackendResult AptBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[apt] 包名校验失败: " + v.reason};

    Logger::step("apt remove " + [&]{
        std::string s;
        for (auto& p : pkgs) s += p + " ";
        return s;
    }());

    std::vector<std::string> args = {"remove", "-y"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;
    opts.extraEnv["DEBIAN_FRONTEND"] = "noninteractive";

    int rc = ExecEngine::run("apt", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("apt remove 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult AptBackend::upgrade() {
    Logger::step("apt update && apt upgrade -y");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;
    opts.extraEnv["DEBIAN_FRONTEND"] = "noninteractive";

    // 先更新索引
    int rc = ExecEngine::run("apt", {"update"}, opts);
    if (rc != 0) return {rc, "apt update 失败"};

    rc = ExecEngine::run("apt", {"upgrade", "-y"}, opts);
    if (rc == 0) Logger::ok("系统更新完成");
    else Logger::error("apt upgrade 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult AptBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("apt-cache", {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string AptBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture(
        "dpkg-query",
        {"-W", "-f=${Version}", "--", pkg},
        opts
    );
    if (!r.ok()) return "";
    // 去除尾部换行
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
