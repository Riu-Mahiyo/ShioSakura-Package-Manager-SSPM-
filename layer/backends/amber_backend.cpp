// ═══════════════════════════════════════════════════════════
//  layer/backends/amber_backend.cpp — Amber PM Backend
//  架构文档第 6 节：Amber PM 集成
//
//  官方网站：https://amber-pm.spark-app.store
//  安装路径：/usr/local/bin/amber（系统级）
//            ~/.local/bin/amber（用户级）
//
//  工作流程（架构文档规定）：
//    1. 读取 token（~/.config/sspm/amber.token）
//    2. 调用 amber CLI 执行操作（API 通过官方工具封装）
//       OR 直接调用 Amber HTTP API（如果 CLI 不可用）
//    3. 校验下载包（SHA256）
//    4. 安装失败 → rollback（删除已解压文件）
//    5. 写入 SkyDB（由 cli_router 统一处理）
//
//  注意：Amber PM 的 CLI 会话最终以 amber install <pkg> 形式执行
//  如果 amber CLI 不存在，显示安装指引
// ═══════════════════════════════════════════════════════════
#include "amber_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace SSPM {
namespace fs = std::filesystem;

// ── Amber API base URL（来自架构文档）────────────────────
std::string AmberBackend::apiBase() {
    const char* env = getenv("AMBER_API");
    if (env && *env) return std::string(env);
    return "https://amber-pm.spark-app.store/api/v1";
}

// ── Token 管理 ────────────────────────────────────────────
std::string AmberBackend::token() {
    const char* home = getenv("HOME");
    if (!home) return "";

    // 优先环境变量
    const char* envtok = getenv("AMBER_TOKEN");
    if (envtok && *envtok) return std::string(envtok);

    // 读文件
    std::string path = std::string(home) + "/.config/sspm/amber.token";
    std::ifstream f(path);
    if (!f) return "";
    std::string tok;
    std::getline(f, tok);
    // trim whitespace
    while (!tok.empty() && (tok.back() == '\n' || tok.back() == '\r' || tok.back() == ' '))
        tok.pop_back();
    return tok;
}

bool AmberBackend::setToken(const std::string& tok) {
    const char* home = getenv("HOME");
    if (!home) return false;
    std::string dir  = std::string(home) + "/.config/sky";
    std::string path = dir + "/amber.token";
    try { fs::create_directories(dir); } catch (...) { return false; }
    std::ofstream f(path);
    if (!f) return false;
    f << tok << "\n";
    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write);
    Logger::ok("Amber token 已保存到 " + path);
    return true;
}

// ── 可用性检测 ────────────────────────────────────────────
bool AmberBackend::available() const {
    // amber CLI 存在
    return ExecEngine::exists("amber");
}

// ── 包元数据（stub：实际通过 amber CLI 查询）────────────
AmberPkgMeta AmberBackend::fetchMeta(const std::string& pkg) {
    // 通过 amber info <pkg> --json 获取元数据
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 15;

    auto r = ExecEngine::capture("amber", {"info", "--json", "--", pkg}, opts);
    AmberPkgMeta meta;
    meta.name = pkg;

    if (!r.ok() || r.stdout_out.empty()) return meta;

    // 解析简单 JSON（只提取 version 和 sha256）
    auto& out = r.stdout_out;
    auto extractField = [&](const std::string& key) -> std::string {
        auto pos = out.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        pos = out.find('"', pos + key.size() + 3);
        if (pos == std::string::npos) return "";
        auto end = out.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return out.substr(pos + 1, end - pos - 1);
    };

    meta.version     = extractField("version");
    meta.sha256      = extractField("sha256");
    meta.downloadUrl = extractField("download_url");
    meta.description = extractField("description");
    return meta;
}

bool AmberBackend::downloadAndVerify(const AmberPkgMeta& meta,
                                      const std::string& destPath) {
    if (meta.downloadUrl.empty()) {
        Logger::error("[amber] 无法获取下载 URL");
        return false;
    }

    Logger::step("下载: " + meta.downloadUrl);
    // amber CLI 负责实际下载；我们通过 curl 验证可达性
    // 实际下载由 amber install 命令处理
    // 这里只做连通性检测
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 300;
    // 用 curl 下载到 destPath
    auto curlR = ExecEngine::capture("curl",
        {"-fsSL", "-o", destPath, "--", meta.downloadUrl}, opts);
    if (!curlR.ok()) {
        Logger::error("[amber] 下载失败 exit=" + std::to_string(curlR.exitCode));
        return false;
    }

    // SHA256 校验
    if (!meta.sha256.empty()) {
        Logger::step("校验 SHA256...");
        ExecEngine::Options opts;
        opts.captureOutput = true;
        opts.timeoutSec    = 10;

        auto res = ExecEngine::capture("sha256sum", {"--", destPath}, opts);
        if (res.ok()) {
            std::string actual = res.stdout_out.substr(0, 64);
            if (actual == meta.sha256) {
                Logger::ok("SHA256 校验通过");
            } else {
                Logger::error("[amber] SHA256 不匹配！");
                Logger::error("  期望: " + meta.sha256);
                Logger::error("  实际: " + actual);
                fs::remove(destPath);
                return false;
            }
        } else {
            Logger::warn("sha256sum 不可用，跳过校验");
        }
    }
    return true;
}

BackendResult AmberBackend::installFile(const std::string& path) {
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;
    int rc = ExecEngine::run("amber", {"install", "--", path}, opts);
    return {rc, rc == 0 ? "" : "amber install 失败"};
}

// ── 主操作 ────────────────────────────────────────────────

BackendResult AmberBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[amber] 包名校验失败: " + v.reason};

    Logger::step("amber install " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    // 提示 token（如果未设置）
    std::string tok = token();
    if (tok.empty()) {
        Logger::warn("Amber token 未设置");
        Logger::info("请访问 https://amber-pm.spark-app.store 获取 token");
        Logger::info("然后运行: sspm amber-token <your-token>");
    }

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    // 设置 token 环境变量（amber CLI 会读取）
    if (!tok.empty()) {
        opts.extraEnv["AMBER_TOKEN"] = tok;
    }

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        int rc = ExecEngine::run("amber", {"install", "--", pkg}, opts);
        if (rc == 0) Logger::ok("已安装: " + pkg);
        else {
            Logger::error("amber install " + pkg + " 失败 exit=" + std::to_string(rc));
            lastRc = rc;
        }
    }
    return {lastRc, ""};
}

BackendResult AmberBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[amber] 包名校验失败: " + v.reason};

    Logger::step("amber remove " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 60;

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        int rc = ExecEngine::run("amber", {"remove", "--", pkg}, opts);
        if (rc == 0) Logger::ok("已卸载: " + pkg);
        else {
            Logger::error("amber remove " + pkg + " 失败");
            lastRc = rc;
        }
    }
    return {lastRc, ""};
}

BackendResult AmberBackend::upgrade() {
    Logger::step("amber update");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("amber", {"update"}, opts);
    if (rc == 0) Logger::ok("Amber 包更新完成");
    else Logger::error("amber update 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult AmberBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 20;

    auto r = ExecEngine::capture("amber", {"search", "--", query}, opts);
    if (!r.ok() || r.stdout_out.empty()) {
        Logger::info("Amber PM 搜索: https://amber-pm.spark-app.store/search?q=" + query);
        return {r.exitCode, "（请访问官网搜索: https://amber-pm.spark-app.store）\n"};
    }
    return {r.exitCode, r.stdout_out};
}

std::string AmberBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture("amber", {"info", "--version", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
