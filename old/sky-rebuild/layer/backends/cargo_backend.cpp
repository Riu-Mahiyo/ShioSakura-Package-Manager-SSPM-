// layer/backends/cargo_backend.cpp — Rust cargo 包管理器
#include "cargo_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <unistd.h>

namespace SSPM {
namespace fs = std::filesystem;

std::string CargoBackend::cargoBin() {
    // 优先 PATH 中的 cargo
    if (ExecEngine::exists("cargo")) return "cargo";
    // 用户级安装路径
    const char* home = getenv("HOME");
    if (home) {
        std::string p = std::string(home) + "/.cargo/bin/cargo";
        if (fs::exists(p)) return p;
    }
    return "cargo";
}

bool CargoBackend::available() const {
    return ExecEngine::exists("cargo") || []{
        const char* h = getenv("HOME");
        return h && fs::exists(std::string(h) + "/.cargo/bin/cargo");
    }();
}

BackendResult CargoBackend::install(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[cargo] 包名校验失败: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;  // 编译可能较慢

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        Logger::step("cargo install " + pkg);
        int rc = ExecEngine::run(cargoBin(), {"install", "--", pkg}, opts);
        if (rc == 0) Logger::ok("已安装: " + pkg);
        else {
            Logger::error("cargo install " + pkg + " 失败 exit=" + std::to_string(rc));
            lastRc = rc;
        }
    }
    return {lastRc, ""};
}

BackendResult CargoBackend::remove(const std::vector<std::string>& pkgs) {
    auto v = InputValidator::pkgList(pkgs);
    if (!v) return {1, "[cargo] 包名校验失败: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 60;

    int lastRc = 0;
    for (auto& pkg : pkgs) {
        int rc = ExecEngine::run(cargoBin(), {"uninstall", "--", pkg}, opts);
        if (rc == 0) Logger::ok("已卸载: " + pkg);
        else { Logger::error("cargo uninstall " + pkg + " 失败"); lastRc = rc; }
    }
    return {lastRc, ""};
}

BackendResult CargoBackend::upgrade() {
    // cargo install-update 需要安装 cargo-update crate
    if (ExecEngine::exists("cargo-install-update")) {
        Logger::step("cargo install-update -a");
        ExecEngine::Options opts;
        opts.captureOutput = false;
        opts.timeoutSec    = 1200;
        int rc = ExecEngine::run("cargo", {"install-update", "-a"}, opts);
        return {rc, ""};
    }

    // 回退：提示安装 cargo-update
    Logger::warn("升级全部 cargo 包需要 cargo-update crate");
    Logger::info("安装方法: sspm install cargo-update --backend=cargo");
    Logger::info("或手动: cargo install cargo-update");
    return {0, ""};
}

BackendResult CargoBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法"};

    Logger::step("cargo search " + query);
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 20;

    int rc = ExecEngine::run(cargoBin(), {"search", "--", query}, opts);
    return {rc, ""};
}

std::string CargoBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    // cargo install --list | grep "^<pkg> "
    auto r = ExecEngine::capture(cargoBin(), {"install", "--list"}, opts);
    if (!r.ok()) return "";

    std::istringstream ss(r.stdout_out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.rfind(pkg + " ", 0) == 0) {
            // 格式：ripgrep v13.0.0:
            auto v_pos = line.find(" v");
            auto colon = line.rfind(':');
            if (v_pos != std::string::npos) {
                std::string ver = line.substr(v_pos + 2);
                if (colon != std::string::npos && colon > v_pos)
                    ver = ver.substr(0, colon - v_pos - 2);
                return ver;
            }
        }
    }
    return "";
}

} // namespace SSPM
