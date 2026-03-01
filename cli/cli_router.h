#pragma once
// ═══════════════════════════════════════════════════════════
//  cli/cli_router.h — CLI 命令路由（v2.3 完整版）
//
//  命令清单：
//    install / remove / upgrade / search / list
//    doctor  / version / help
//    backends        — 列出所有 backend 及可用性
//    db [list|query <pkg>|orphan]  — SkyDB 管理
//    lock            — 显示安装锁状态
//    amber-token <t> — 设置 Amber PM token
//    fix-path [nix|brew]  — 修复 PATH 配置
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include <set>

namespace SSPM {

class CliRouter {
public:
    static int dispatch(int argc, char* argv[]);

private:
    // ── 基础命令 ─────────────────────────────────────────
    static int cmdInstall(const std::vector<std::string>& pkgs,
                          const std::string& backend);
    static int cmdRemove(const std::vector<std::string>& pkgs,
                         const std::string& backend);
    static int cmdUpgrade(const std::string& backend);
    static int cmdSearch(const std::string& query,
                         const std::string& backend);
    static int cmdList();
    static int cmdDoctor();
    static int cmdVersion();
    static int cmdHelp();

    // ── v2.3 新命令 ──────────────────────────────────────
    static int cmdBackends();
    static int cmdDb(const std::string& sub, const std::string& pkg);
    static int cmdLockStatus();
    static int cmdAmberToken(const std::string& token);

    static void printUsage();
};

} // namespace SSPM
