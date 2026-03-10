#pragma once
// ═══════════════════════════════════════════════════════════
//  layer/backend.h — SSPM Backend 抽象接口（完整版 v3.0）
//  架构文档第三节：Backend Driver 抽象层
//
//  必须实现：install / remove / upgrade / search
//  建议实现：query / listInstalled / autoremove / dryRun
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include "../core/exec_engine.h"

namespace SSPM {

struct BackendResult {
    int         exitCode = 0;
    std::string output;
    std::string errorMessage;
    bool ok() const { return exitCode == 0; }
};

struct InstalledPkg {
    std::string name;
    std::string version;
    std::string arch;
    std::string description;
    bool        isExplicit = true;
};

struct DryRunStep {
    std::string action;
    std::string pkg;
    std::string version;
    long        sizeBytes = 0;
};

class LayerBackend {
public:
    virtual ~LayerBackend() = default;

    virtual std::string name() const = 0;
    virtual bool available() const   = 0;

    // 平台限制（默认全平台）
    virtual bool onlyOnLinux()   const { return false; }
    virtual bool onlyOnMacOS()   const { return false; }
    virtual bool onlyOnFreeBSD() const { return false; }

    // ── 核心操作（必须实现）──────────────────────────────
    virtual BackendResult install(const std::vector<std::string>& pkgs) = 0;
    virtual BackendResult remove(const std::vector<std::string>& pkgs)  = 0;
    virtual BackendResult upgrade()                                      = 0;
    virtual BackendResult search(const std::string& query)               = 0;

    // ── 扩展操作（有默认实现，建议覆盖）────────────────
    virtual BackendResult query(const std::string& pkg) { return search(pkg); }
    virtual std::vector<InstalledPkg> listInstalled() { return {}; }
    virtual BackendResult autoremove() { return {0, "(不支持)", ""}; }
    virtual std::vector<DryRunStep> dryRun(const std::string&,
                                            const std::vector<std::string>&) { return {}; }
    virtual bool        supportsDryRun()  const { return false; }
    virtual std::string installedVersion(const std::string& /*pkg*/) { return ""; }
    virtual bool        requiresRoot()    const { return false; }
};

} // namespace SSPM
