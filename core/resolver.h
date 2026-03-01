#pragma once
// ═══════════════════════════════════════════════════════════
//  core/resolver.h — 依赖图构建器 + 冲突检测器
//  架构文档第二节：Core Layer
//
//  功能：
//    1. 统一依赖图构建（跨 backend）
//    2. 冲突检测（同名包多 backend、文件冲突、ABI 冲突）
//    3. 执行计划生成器（install order 拓扑排序）
//    4. 优先级策略系统
//
//  依赖图：有向无环图（DAG），节点 = 包，边 = 依赖关系
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>

namespace SSPM {

// ── 依赖节点 ──────────────────────────────────────────────
struct PkgNode {
    std::string name;
    std::string version;
    std::string backend;
    std::vector<std::string> deps;      // 直接依赖
    std::vector<std::string> provides;  // 该包提供的虚包名
    bool installed = false;
};

// ── 冲突类型 ──────────────────────────────────────────────
enum class ConflictType {
    NameConflict,   // 同名包来自不同 backend
    FileConflict,   // 安装文件路径重叠
    AbiConflict,    // 动态库版本不兼容
    VersionConflict, // 依赖版本范围不满足
};

struct Conflict {
    ConflictType type;
    std::string  pkgA;
    std::string  pkgB;
    std::string  description;
    std::string  suggestion;
};

// ── 执行计划 ──────────────────────────────────────────────
struct InstallStep {
    enum class Action { Install, Remove, Upgrade, Skip };
    Action      action;
    std::string pkg;
    std::string backend;
    std::string version;
    int         priority = 0;  // 越高越先执行
};

// ── 优先级策略 ────────────────────────────────────────────
struct BackendPriority {
    std::string backend;
    int         score;  // 越高越优先选择此 backend
};

class DependencyResolver {
public:
    // 初始化（加载当前安装状态）
    void loadInstalled(const std::vector<PkgNode>& installed);

    // 构建依赖图
    // 从 backend 查询真实依赖（apt-cache depends 等）
    void buildGraph(const std::vector<std::string>& pkgs,
                    const std::string& backend);

    // ── 冲突检测 ──────────────────────────────────────────
    std::vector<Conflict> detectConflicts(
        const std::vector<std::string>& pkgs,
        const std::string& targetBackend);

    // ── 执行计划生成 ──────────────────────────────────────
    // 拓扑排序后生成按序安装步骤
    std::vector<InstallStep> buildInstallPlan(
        const std::vector<std::string>& pkgs,
        const std::string& backend);

    // ── 优先级策略 ────────────────────────────────────────
    // 根据系统配置推荐最优 backend
    std::string selectBestBackend(const std::string& pkg,
                                  const std::vector<BackendPriority>& priorities);

    // ── 依赖树可视化 ──────────────────────────────────────
    // 返回 ASCII 树形结构
    std::string visualizeTree(const std::string& rootPkg, int maxDepth = 5);

    // ── 孤包检测 ──────────────────────────────────────────
    // 返回没有任何其他包依赖它的包（用户可选择卸载）
    std::vector<std::string> findOrphans();

private:
    std::map<std::string, PkgNode> graph_;   // name → node
    std::set<std::string>           visited_;

    void dfs(const std::string& pkg, std::vector<InstallStep>& plan,
             const std::string& backend);
    bool hasCycle(const std::string& pkg, std::set<std::string>& path);
};

// ── 全局优先级配置（可通过 sspm.conf 配置）────────────────
struct PriorityConfig {
    // backend 偏好顺序（index 越小优先级越高）
    std::vector<std::string> preferredOrder = {
        "apt", "dnf", "pacman", "zypper", "portage",
        "flatpak", "snap", "nix",
        "npm", "pip", "cargo", "gem",
        "brew", "macports",
        "spk", "amber"
    };

    // 禁用的 backend（用户配置）
    std::set<std::string> disabled;

    // 每个发行版的默认 backend
    static std::string defaultForDistro(const std::string& distro);
};

} // namespace SSPM
