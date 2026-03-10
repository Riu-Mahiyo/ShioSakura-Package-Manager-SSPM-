// layer/backends/dpkg_backend.cpp — dpkg 直接操作 .deb 文件
// 设计：
//   sspm install --backend=dpkg ./pkg.deb     → dpkg -i ./pkg.deb
//   sspm remove  --backend=dpkg pkg-name      → dpkg -r pkg-name
//   sky search  --backend=dpkg query         → dpkg -l *query*
#include "dpkg_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool DpkgBackend::available() const {
    return ExecEngine::exists("dpkg");
}

BackendResult DpkgBackend::install(const std::vector<std::string>& pkgs) {
    // pkgs 可以是 .deb 文件路径，校验用 filePath 或 pkg
    for (auto& p : pkgs) {
        // 以 / 或 ./ 开头 = 文件路径；否则 = 包名（重新配置已解压包用）
        if (p.empty()) return {1, "[dpkg] 空包名/路径"};
        bool isPath = (p[0] == '/' || p.rfind("./", 0) == 0 || p.rfind("../", 0) == 0);
        if (isPath) {
            auto v = InputValidator::filePath(p, false, true);
            if (!v) return {1, "[dpkg] 文件路径非法: " + v.reason};
        } else {
            auto v = InputValidator::pkg(p);
            if (!v) return {1, "[dpkg] 包名非法: " + v.reason};
        }
    }

    Logger::step("dpkg -i " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"-i"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;
    opts.extraEnv["DEBIAN_FRONTEND"] = "noninteractive";

    int rc = ExecEngine::run("dpkg", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else {
        Logger::error("dpkg -i 失败 exit=" + std::to_string(rc));
        Logger::info("提示：若有依赖缺失，运行 apt install -f 修复");
    }
    return {rc, ""};
}

BackendResult DpkgBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[dpkg] 包名校验失败: " + v.reason};

    Logger::step("dpkg -r " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"-r"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 60;
    opts.extraEnv["DEBIAN_FRONTEND"] = "noninteractive";

    int rc = ExecEngine::run("dpkg", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("dpkg -r 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult DpkgBackend::upgrade() {
    // dpkg 没有 upgrade 概念，提示用 apt
    Logger::warn("dpkg 不支持全局升级，请使用 sky upgrade --backend=apt");
    return {0, ""};
}

BackendResult DpkgBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    // dpkg -l *query* 列出匹配已安装包
    auto r = ExecEngine::capture("dpkg", {"-l", "*" + query + "*"}, opts);
    return {r.exitCode, r.stdout_out.empty() ? "(未找到匹配的已安装包)\n" : r.stdout_out};
}

std::string DpkgBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;

    auto r = ExecEngine::capture(
        "dpkg-query", {"-W", "-f=${Version}", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
