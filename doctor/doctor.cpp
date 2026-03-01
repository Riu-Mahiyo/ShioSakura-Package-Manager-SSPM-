// ═══════════════════════════════════════════════════════════
//  doctor/doctor.cpp — 系统健康检查（v2.3）
// ═══════════════════════════════════════════════════════════
#include "doctor.h"
#include "../core/detect.h"
#include "../core/logger.h"
#include "../core/exec_engine.h"
#include "../layer/layer_manager.h"
#include "../layer/backends/brew_backend.h"
#include "../layer/backends/nix_backend.h"
#include "../layer/backends/flatpak_backend.h"
#include "../layer/backends/snap_backend.h"
#include "../db/skydb.h"

#include <iostream>
#include <cstdlib>

namespace SSPM {

// ── 基础检查 ─────────────────────────────────────────────

DoctorIssue Doctor::checkRoot() {
    if (Detect::isRoot())
        return {DoctorIssue::Severity::OK, "running as root"};
    return {DoctorIssue::Severity::Warn,
            "not running as root",
            "安装/删除包时需要 sudo 或以 root 运行"};
}

DoctorIssue Doctor::checkBackend() {
    std::string be = Detect::defaultBackend();
    if (!be.empty())
        return {DoctorIssue::Severity::OK, "backend detected: " + be};

    auto all = Detect::availableBackends();
    if (!all.empty()) {
        std::string found;
        for (auto& b : all) found += b + " ";
        return {DoctorIssue::Severity::Warn,
                "no default backend for this OS, but found: " + found,
                "用 sky --backend=<name> 指定"};
    }
    return {DoctorIssue::Severity::Error,
            "no supported backend found",
            "请安装 apt / pacman / dnf / zypper / brew / pkg 之一"};
}

DoctorIssue Doctor::checkNetwork() {
    if (Detect::networkReachable())
        return {DoctorIssue::Severity::OK, "network reachable"};
    return {DoctorIssue::Severity::Warn,
            "network unreachable (8.8.8.8 ping failed)",
            "离线模式：只可使用本地已缓存的包"};
}

DoctorIssue Doctor::checkPathConfig() {
    if (Detect::hasCommand("sspm"))
        return {DoctorIssue::Severity::OK, "sky is in PATH"};
    return {DoctorIssue::Severity::Warn,
            "sky not found in PATH",
            "将 sky 安装到 /usr/local/bin 或添加到 PATH"};
}

DoctorIssue Doctor::checkDockerAvailable() {
    if (Detect::hasCommand("docker")) {
        // 检查 docker daemon 是否运行
        ExecEngine::Options opts;
        opts.captureOutput = true;
        opts.timeoutSec    = 5;
        auto r = ExecEngine::capture("docker", {"info"}, opts);
        if (r.ok())
            return {DoctorIssue::Severity::OK, "docker available and running"};
        return {DoctorIssue::Severity::Warn,
                "docker installed but daemon not running",
                "运行 'systemctl start docker' 或 'sudo dockerd'"};
    }
    return {DoctorIssue::Severity::OK, "docker not installed (optional)"};
}

DoctorIssue Doctor::checkSkyDB() {
    SkyDB::load();
    auto all = SkyDB::all();
    if (all.empty())
        return {DoctorIssue::Severity::Warn,
                "skydb empty (no packages recorded)",
                "安装第一个包后 SkyDB 会自动初始化"};
    return {DoctorIssue::Severity::OK,
            "skydb ok (" + std::to_string(all.size()) + " packages)"};
}

// ── v2.3 新增检查 ─────────────────────────────────────────

DoctorIssue Doctor::checkNixPath() {
    if (!ExecEngine::exists("nix") && !ExecEngine::exists("nix-env")) {
        // Nix 未安装，检查是否有 nix-profile 但 PATH 没加
        std::string profileBin = NixBackend::nixProfileBin();
        if (!profileBin.empty()) {
            return {DoctorIssue::Severity::Warn,
                    "nix profile found at " + profileBin + " but not in PATH",
                    "运行: sspm doctor --fix-nix-path 或手动添加 " + profileBin + " 到 PATH",
                    true};
        }
        return {DoctorIssue::Severity::OK, "nix not installed (optional)"};
    }
    // nix 可用：检查版本
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;
    auto r = ExecEngine::capture("nix", {"--version"}, opts);
    std::string ver = r.ok() ? r.stdout_out.substr(0, r.stdout_out.find('\n')) : "unknown";
    return {DoctorIssue::Severity::OK, "nix available: " + ver};
}

DoctorIssue Doctor::checkBrewPath() {
    std::string prefix = BrewBackend::brewPrefix();
    if (prefix.empty()) {
        return {DoctorIssue::Severity::OK, "brew not installed (optional)"};
    }
    std::string binPath = prefix + "/bin";
    const char* path = getenv("PATH");
    if (path && std::string(path).find(binPath) != std::string::npos)
        return {DoctorIssue::Severity::OK, "brew PATH ok: " + binPath};
    return {DoctorIssue::Severity::Warn,
            "brew installed at " + prefix + " but " + binPath + " not in PATH",
            "运行: export PATH=\"" + binPath + ":$PATH\"  或  sspm doctor --fix-brew-path",
            true};
}

DoctorIssue Doctor::checkFlatpakRemote() {
    if (!ExecEngine::exists("flatpak"))
        return {DoctorIssue::Severity::OK, "flatpak not installed (optional)"};

    if (FlatpakBackend::hasFlathub())
        return {DoctorIssue::Severity::OK, "flatpak: Flathub remote configured"};

    return {DoctorIssue::Severity::Warn,
            "flatpak installed but Flathub remote not configured",
            "运行: flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo",
            true};
}

DoctorIssue Doctor::checkSnapdDaemon() {
    if (!ExecEngine::exists("snap"))
        return {DoctorIssue::Severity::OK, "snap not installed (optional)"};

    if (SnapBackend::snapdRunning())
        return {DoctorIssue::Severity::OK, "snapd daemon running"};

    return {DoctorIssue::Severity::Warn,
            "snap installed but snapd daemon not running",
            "运行: sudo systemctl start snapd"};
}

DoctorIssue Doctor::checkNpmPrefix() {
    if (!ExecEngine::exists("npm"))
        return {DoctorIssue::Severity::OK, "npm not installed (optional)"};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    auto r = ExecEngine::capture("npm", {"config", "get", "prefix"}, opts);
    if (!r.ok())
        return {DoctorIssue::Severity::Warn,
                "npm installed but 'npm config get prefix' failed"};

    auto prefix = r.stdout_out;
    while (!prefix.empty() && (prefix.back() == '\n' || prefix.back() == '\r'))
        prefix.pop_back();

    // 检查 prefix/bin 是否在 PATH
    std::string binPath = prefix + "/bin";
    const char* path = getenv("PATH");
    if (path && std::string(path).find(binPath) != std::string::npos)
        return {DoctorIssue::Severity::OK, "npm prefix ok: " + prefix};

    return {DoctorIssue::Severity::Warn,
            "npm global bin (" + binPath + ") not in PATH",
            "运行: export PATH=\"" + binPath + ":$PATH\""};
}

DoctorIssue Doctor::checkPipStatus() {
    bool hasPip3 = ExecEngine::exists("pip3");
    bool hasPip  = ExecEngine::exists("pip");

    if (!hasPip3 && !hasPip)
        return {DoctorIssue::Severity::OK, "pip not installed (optional)"};

    std::string pip = hasPip3 ? "pip3" : "pip";

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    auto r = ExecEngine::capture(pip, {"--version"}, opts);
    std::string ver = r.ok() ? r.stdout_out.substr(0, r.stdout_out.find('\n')) : "unknown";

    // 检查是否在 venv 中（VIRTUAL_ENV 环境变量）
    const char* venv = getenv("VIRTUAL_ENV");
    if (venv && *venv)
        return {DoctorIssue::Severity::OK,
                pip + " ok (in venv: " + std::string(venv) + ")"};

    return {DoctorIssue::Severity::OK, pip + " available: " + ver};
}

// ── 主入口 ────────────────────────────────────────────────

std::vector<DoctorIssue> Doctor::runAll() {
    std::vector<DoctorIssue> issues;

    // 基础
    issues.push_back(checkRoot());
    issues.push_back(checkBackend());
    issues.push_back(checkNetwork());
    issues.push_back(checkPathConfig());
    issues.push_back(checkSkyDB());

    // v2.3 新增
    issues.push_back(checkNixPath());
    issues.push_back(checkBrewPath());
    issues.push_back(checkFlatpakRemote());
    issues.push_back(checkSnapdDaemon());
    issues.push_back(checkNpmPrefix());
    issues.push_back(checkPipStatus());
    issues.push_back(checkDockerAvailable());

    return issues;
}

void Doctor::printReport(const std::vector<DoctorIssue>& issues) {
    std::cout << "\n";
    int errors = 0, warns = 0;
    for (auto& issue : issues) {
        switch (issue.severity) {
            case DoctorIssue::Severity::OK:
                Logger::ok(issue.message);
                break;
            case DoctorIssue::Severity::Warn:
                Logger::warn(issue.message);
                if (!issue.fix.empty())
                    Logger::info("   → " + issue.fix);
                warns++;
                break;
            case DoctorIssue::Severity::Error:
                Logger::error(issue.message);
                if (!issue.fix.empty())
                    Logger::info("   → " + issue.fix);
                errors++;
                break;
        }
    }
    std::cout << "\n";
    if (errors == 0 && warns == 0) {
        std::cout << C_GREEN C_BOLD "✓ All checks passed\n" C_RESET;
    } else {
        if (errors > 0) std::cout << C_RED   << errors << " error(s)  " C_RESET;
        if (warns  > 0) std::cout << C_YELLOW << warns  << " warning(s)" C_RESET;
        std::cout << "\n";
    }
    std::cout << "\n";
}

} // namespace SSPM
