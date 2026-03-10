// layer/backends/pip_backend.cpp — pip packages
#include "pip_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"
#include <sstream>

namespace SSPM {

std::string PipBackend::pipCmd() {
    if (ExecEngine::exists("pip3")) return "pip3";
    if (ExecEngine::exists("pip"))  return "pip";
    return "";
}

bool PipBackend::available() const {
    return !pipCmd().empty();
}

BackendResult PipBackend::install(const std::vector<std::string>& pkgs) {
    // pip 包名允许 [a-zA-Z0-9._-] 和版本说明符（如 requests>=2.0）
    for (auto& p : pkgs) {
        auto v = InputValidator::safeString(p, 256);
        if (!v) return {1, "[pip] 包名非法: " + v.reason};
    }

    std::string pip = pipCmd();
    Logger::step(pip + " install " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    // --user：不需要 root，安装到用户目录
    std::vector<std::string> args = {"install", "--user"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run(pip, args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else {
        // 部分系统 pip 要求 --break-system-packages（Debian 12+）
        Logger::warn("尝试 --break-system-packages...");
        args.push_back("--break-system-packages");
        rc = ExecEngine::run(pip, args, opts);
        if (rc == 0) Logger::ok("安装成功（--break-system-packages）");
        else Logger::error("pip 安装失败 exit=" + std::to_string(rc));
    }
    return {rc, ""};
}

BackendResult PipBackend::remove(const std::vector<std::string>& pkgs) {
    for (auto& p : pkgs) {
        auto v = InputValidator::safeString(p, 128);
        if (!v) return {1, "[pip] 包名非法: " + v.reason};
    }

    std::string pip = pipCmd();
    Logger::step(pip + " uninstall -y " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"uninstall", "-y"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run(pip, args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("pip uninstall 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PipBackend::upgrade() {
    std::string pip = pipCmd();
    Logger::step(pip + " list --outdated  →  " + pip + " install -U");

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 60;

    // 获取过时包列表
    auto r = ExecEngine::capture(pip,
        {"list", "--outdated", "--format=freeze"}, opts);
    if (!r.ok() || r.stdout_out.empty()) {
        Logger::ok("没有需要更新的 pip 包");
        return {0, ""};
    }

    // 解析出包名（格式: name==version）
    std::vector<std::string> toUpdate;
    std::istringstream ss(r.stdout_out);
    std::string line;
    while (std::getline(ss, line)) {
        auto eq = line.find("==");
        if (eq != std::string::npos) {
            toUpdate.push_back(line.substr(0, eq));
        }
    }

    if (toUpdate.empty()) {
        Logger::ok("没有需要更新的 pip 包");
        return {0, ""};
    }

    Logger::step("更新 " + std::to_string(toUpdate.size()) + " 个包...");
    std::vector<std::string> upgradeArgs = {"install", "--user", "-U"};
    upgradeArgs.insert(upgradeArgs.end(), toUpdate.begin(), toUpdate.end());

    ExecEngine::Options upgradeOpts;
    upgradeOpts.captureOutput = false;
    upgradeOpts.timeoutSec    = 600;

    int rc = ExecEngine::run(pip, upgradeArgs, upgradeOpts);
    if (rc == 0) Logger::ok("pip 包更新完成");
    else Logger::error("pip upgrade 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult PipBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    // pip search 在新版本已被禁用，改用 pip index versions 或提示
    Logger::warn("pip search 已被禁用，请访问 https://pypi.org/search/?q=" + query);

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 15;

    // 尝试 pip index versions <pkg>（pip 21.2+）
    std::string pip = pipCmd();
    auto r = ExecEngine::capture(pip, {"index", "versions", "--", query}, opts);
    return {r.exitCode, r.stdout_out.empty() ?
        "提示: 请访问 https://pypi.org/search/?q=" + query + "\n" :
        r.stdout_out};
}

std::string PipBackend::installedVersion(const std::string& pkg) {
    std::string pip = pipCmd();
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    // pip show <pkg> — 输出包含 "Version: x.y.z"
    auto r = ExecEngine::capture(pip, {"show", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto& out = r.stdout_out;
    auto pos = out.find("Version: ");
    if (pos == std::string::npos) return "";
    auto start = pos + 9;
    auto nl    = out.find('\n', start);
    return out.substr(start, (nl != std::string::npos) ? nl - start : std::string::npos);
}

} // namespace SSPM
