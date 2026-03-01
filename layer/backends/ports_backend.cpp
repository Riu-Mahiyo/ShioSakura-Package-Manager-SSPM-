// layer/backends/ports_backend.cpp — FreeBSD Ports Collection
#include "ports_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

#include <filesystem>
#include <sstream>

namespace SSPM {
namespace fs = std::filesystem;

bool PortsBackend::available() const {
    // 严格限制：只在 FreeBSD 上启用
    // 通过检测 /usr/ports 目录和 FreeBSD 系统标识
#ifdef __FreeBSD__
    return fs::exists("/usr/ports");
#else
    return false;
#endif
}

bool PortsBackend::findPort(const std::string& pkg, std::string& portDir) {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 15;

    // 用 whereis 或 /usr/sbin/pkg search 查找 port 路径
    auto r = ExecEngine::capture("whereis", {"-q", "-s", "--", pkg}, opts);
    if (r.ok() && !r.stdout_out.empty()) {
        std::string path = r.stdout_out;
        while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
            path.pop_back();
        if (!path.empty() && fs::exists(path)) {
            portDir = path;
            return true;
        }
    }

    // 搜索 /usr/ports（简单遍历）
    if (fs::exists("/usr/ports")) {
        for (auto& category : fs::directory_iterator("/usr/ports")) {
            std::string candidate = category.path().string() + "/" + pkg;
            if (fs::exists(candidate + "/Makefile")) {
                portDir = candidate;
                return true;
            }
        }
    }

    return false;
}

BackendResult PortsBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[ports] 包名校验失败: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 3600;  // 源码编译可能很慢

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        std::string portDir;
        if (!findPort(pkg, portDir)) {
            Logger::error("[ports] 找不到 port: " + pkg);
            Logger::info("请确认 /usr/ports 已更新: portsnap fetch update");
            lastRc = 1;
            continue;
        }

        Logger::step("make -C " + portDir + " install clean BATCH=yes");
        int rc = ExecEngine::run("make",
            {"-C", portDir, "install", "clean", "BATCH=yes"}, opts);
        if (rc == 0) Logger::ok("已安装: " + pkg);
        else { Logger::error("ports install " + pkg + " 失败"); lastRc = rc; }
    }
    return {lastRc, ""};
}

BackendResult PortsBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[ports] 包名校验失败: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        std::string portDir;
        if (!findPort(pkg, portDir)) {
            // 回退：使用 pkg 删除
            Logger::info("[ports] 用 pkg 删除 " + pkg);
            int rc = ExecEngine::run("pkg", {"delete", "-y", "--", pkg}, opts);
            if (rc != 0) lastRc = rc;
            continue;
        }
        int rc = ExecEngine::run("make", {"-C", portDir, "deinstall"}, opts);
        if (rc == 0) Logger::ok("已卸载: " + pkg);
        else { Logger::error("ports deinstall " + pkg + " 失败"); lastRc = rc; }
    }
    return {lastRc, ""};
}

BackendResult PortsBackend::upgrade() {
    Logger::step("portsnap fetch update");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 300;

    int rc = ExecEngine::run("portsnap", {"fetch", "update"}, opts);
    if (rc != 0) {
        Logger::warn("portsnap 更新失败，尝试 git 方式");
    }

    // 用 portmaster 升级已安装 ports（如果存在）
    if (ExecEngine::exists("portmaster")) {
        Logger::step("portmaster -a");
        opts.timeoutSec = 7200;
        rc = ExecEngine::run("portmaster", {"-a", "--no-confirm"}, opts);
    } else {
        Logger::info("如需升级所有 ports，请安装 portmaster：");
        Logger::info("  pkg install portmaster");
    }
    return {rc, ""};
}

BackendResult PortsBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法"};

    Logger::step("searching ports: " + query);
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 30;

    // 用 pkg 搜索（更快）
    int rc = ExecEngine::run("pkg", {"search", "--", query}, opts);
    if (rc != 0 && fs::exists("/usr/ports")) {
        // 回退：make search
        Logger::info("也可以使用: make -C /usr/ports search name=" + query);
    }
    return {rc, ""};
}

std::string PortsBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    auto r = ExecEngine::capture("pkg", {"query", "%v", "--", pkg}, opts);
    if (!r.ok()) return "";
    std::string ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r')) ver.pop_back();
    return ver;
}

} // namespace SSPM
