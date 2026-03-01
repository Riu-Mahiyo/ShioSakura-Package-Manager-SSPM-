// layer/backends/wine_backend.cpp — Wine + Winetricks Backend
#include "wine_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"
#include <sstream>

namespace SSPM {

bool WineBackend::wineAvailable() {
    return ExecEngine::exists("wine") || ExecEngine::exists("wine64");
}

bool WineBackend::winetricksAvailable() {
    return ExecEngine::exists("winetricks");
}

std::string WineBackend::wineVersion() {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;
    auto r = ExecEngine::capture("wine", {"--version"}, opts);
    if (!r.ok()) {
        r = ExecEngine::capture("wine64", {"--version"}, opts);
    }
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    auto nl = ver.find('\n');
    if (nl != std::string::npos) ver = ver.substr(0, nl);
    return ver;
}

bool WineBackend::available() const {
    return wineAvailable();
}

BackendResult WineBackend::install(const std::vector<std::string>& pkgs) {
    // winetricks verbs 校验（允许字母数字和 _-）
    for (auto& p : pkgs) {
        auto v = InputValidator::pkg(p);
        if (!v) return {1, "[wine] verb 校验失败: " + v.reason};
    }

    if (!winetricksAvailable()) {
        Logger::error("未找到 winetricks，请先安装: sspm install winetricks");
        return {1, "winetricks not found"};
    }

    Logger::step("winetricks " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    // WINEDEBUG=-all 减少噪音输出
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;
    opts.extraEnv["WINEDEBUG"] = "-all";

    std::vector<std::string> args;
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    int rc = ExecEngine::run("winetricks", args, opts);
    if (rc == 0) Logger::ok("Wine 组件安装成功");
    else Logger::error("winetricks 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult WineBackend::remove(const std::vector<std::string>& pkgs) {
    // winetricks 不支持卸载单个 verb
    // 只能通过 winetricks annihilate 清空整个 Wine prefix
    Logger::warn("Wine 组件不支持单独卸载");
    Logger::info("若需重置 Wine 环境，运行: winetricks annihilate");
    return {0, ""};
}

BackendResult WineBackend::upgrade() {
    Logger::step("winetricks --self-update");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("winetricks", {"--self-update"}, opts);
    if (rc == 0) Logger::ok("winetricks 已更新");
    else Logger::warn("winetricks --self-update 失败（可能已是最新）");
    return {0, ""};  // 非致命
}

BackendResult WineBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    if (!winetricksAvailable()) {
        return {1, "winetricks not found"};
    }

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    // winetricks list-all 输出所有 verb，grep 过滤
    auto r = ExecEngine::capture("winetricks", {"list-all"}, opts);
    if (!r.ok()) return {r.exitCode, r.stdout_out};

    // 过滤含 query 的行
    std::string result;
    std::istringstream ss(r.stdout_out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find(query) != std::string::npos) {
            result += line + "\n";
        }
    }
    return {0, result.empty() ? "(未找到匹配的 Wine 组件)\n" : result};
}

std::string WineBackend::installedVersion(const std::string& pkg) {
    // 返回 wine 版本（组件本身没有版本概念）
    return wineVersion();
}

} // namespace SSPM
