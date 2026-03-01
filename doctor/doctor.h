#pragma once
// ═══════════════════════════════════════════════════════════
//  doctor/doctor.h — 系统健康检查（v2.3）
//
//  输出格式：
//    [OK]   backend detected: apt
//    [OK]   network reachable
//    [WARN] skydb empty
//    [ERR]  no backend found
//
//  v2.3 新增检查（按规划文档）：
//    ✓ nix PATH 检测与修复建议
//    ✓ brew PATH 检测（Linuxbrew）
//    ✓ flatpak remote（Flathub）
//    ✓ snapd daemon
//    ✓ npm prefix 检查
//    ✓ pip venv 冲突检测
//    ✓ docker 可用性
//    ✓ SkyDB 状态
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>

namespace SSPM {

struct DoctorIssue {
    enum class Severity { OK, Warn, Error };

    Severity    severity;
    std::string message;
    std::string fix;
    bool        autoFix = false;
};

class Doctor {
public:
    static std::vector<DoctorIssue> runAll();
    static void printReport(const std::vector<DoctorIssue>& issues);

    // ── 基础检查 ──────────────────────────────────────────
    static DoctorIssue checkRoot();
    static DoctorIssue checkBackend();
    static DoctorIssue checkNetwork();
    static DoctorIssue checkPathConfig();
    static DoctorIssue checkDockerAvailable();
    static DoctorIssue checkSkyDB();

    // ── v2.3 新增 ─────────────────────────────────────────
    static DoctorIssue checkNixPath();
    static DoctorIssue checkBrewPath();
    static DoctorIssue checkFlatpakRemote();
    static DoctorIssue checkSnapdDaemon();
    static DoctorIssue checkNpmPrefix();
    static DoctorIssue checkPipStatus();

    // ── 插件语言环境 + i18n ───────────────────────────────
    static std::vector<DoctorIssue> checkPluginLanguages();
    static DoctorIssue checkI18nPacks();
};

} // namespace SSPM

// NOTE: additional checks added to runAll() in doctor.cpp:
// checkPluginLanguages(), checkI18nPacks()
