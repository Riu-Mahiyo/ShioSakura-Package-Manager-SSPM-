// ═══════════════════════════════════════════════════════════
//  core/resolver.cpp — 依赖图构建器 + 冲突检测 实现
// ═══════════════════════════════════════════════════════════
#include "resolver.h"
#include "exec_engine.h"
#include "logger.h"

#include <sstream>
#include <algorithm>
#include <queue>
#include <functional>

namespace SSPM {

void DependencyResolver::loadInstalled(const std::vector<PkgNode>& installed) {
    for (auto& n : installed) {
        graph_[n.name] = n;
        graph_[n.name].installed = true;
    }
}

// ── 依赖图构建 ────────────────────────────────────────────
void DependencyResolver::buildGraph(const std::vector<std::string>& pkgs,
                                     const std::string& backend) {
    for (auto& pkg : pkgs) {
        if (graph_.count(pkg)) continue;  // 已在图中

        PkgNode node;
        node.name    = pkg;
        node.backend = backend;

        // 查询真实依赖
        ExecEngine::Options opts;
        opts.captureOutput = true;
        opts.timeoutSec    = 10;

        if (backend == "apt") {
            auto r = ExecEngine::capture("apt-cache", {"depends", "--", pkg}, opts);
            if (r.ok()) {
                std::istringstream ss(r.stdout_out);
                std::string line;
                while (std::getline(ss, line)) {
                    // 格式：  Depends: libcurl4
                    if (line.find("Depends: ") != std::string::npos) {
                        auto pos = line.find("Depends: ");
                        std::string dep = line.substr(pos + 9);
                        // 去掉版本约束如 (>= 1.0)
                        auto paren = dep.find(" (");
                        if (paren != std::string::npos) dep = dep.substr(0, paren);
                        if (!dep.empty() && dep[0] != '<' && dep[0] != '>') {
                            node.deps.push_back(dep);
                        }
                    }
                }
            }
        } else if (backend == "pacman") {
            auto r = ExecEngine::capture("pactree", {"-r", pkg}, opts);
            // 简化处理：只取直接依赖
        }

        graph_[pkg] = node;

        // 递归构建（限深度 3 防爆炸）
        static int depth = 0;
        if (depth < 3) {
            ++depth;
            buildGraph(node.deps, backend);
            --depth;
        }
    }
}

// ── 冲突检测 ──────────────────────────────────────────────
std::vector<Conflict> DependencyResolver::detectConflicts(
    const std::vector<std::string>& pkgs,
    const std::string& targetBackend) {

    std::vector<Conflict> conflicts;

    for (auto& pkg : pkgs) {
        // 检查：同名包已从其他 backend 安装
        auto it = graph_.find(pkg);
        if (it != graph_.end() && it->second.installed) {
            if (it->second.backend != targetBackend) {
                Conflict c;
                c.type        = ConflictType::NameConflict;
                c.pkgA        = pkg + " (via " + it->second.backend + ")";
                c.pkgB        = pkg + " (via " + targetBackend + ")";
                c.description = "包 '" + pkg + "' 已通过 " + it->second.backend +
                               " 安装，若再用 " + targetBackend + " 安装可能冲突";
                c.suggestion  = "先执行: sspm remove " + pkg +
                               " --backend=" + it->second.backend;
                conflicts.push_back(c);
            }
        }
    }

    return conflicts;
}

// ── 执行计划生成（拓扑排序）─────────────────────────────
void DependencyResolver::dfs(const std::string& pkg,
                              std::vector<InstallStep>& plan,
                              const std::string& backend) {
    if (visited_.count(pkg)) return;
    visited_.insert(pkg);

    auto it = graph_.find(pkg);
    if (it != graph_.end()) {
        for (auto& dep : it->second.deps) {
            dfs(dep, plan, backend);
        }
    }

    // 跳过已安装
    bool alreadyInstalled = (it != graph_.end() && it->second.installed &&
                              it->second.backend == backend);

    InstallStep step;
    step.action  = alreadyInstalled ? InstallStep::Action::Skip
                                    : InstallStep::Action::Install;
    step.pkg     = pkg;
    step.backend = backend;
    step.version = (it != graph_.end()) ? it->second.version : "";
    plan.push_back(step);
}

std::vector<InstallStep> DependencyResolver::buildInstallPlan(
    const std::vector<std::string>& pkgs,
    const std::string& backend) {

    std::vector<InstallStep> plan;
    visited_.clear();

    for (auto& pkg : pkgs) {
        dfs(pkg, plan, backend);
    }

    // 移除 Skip 步骤（在显示时保留用于信息展示）
    return plan;
}

// ── 依赖树可视化 ──────────────────────────────────────────
std::string DependencyResolver::visualizeTree(const std::string& rootPkg,
                                               int maxDepth) {
    std::string result;

    std::function<void(const std::string&, int, bool)> draw =
        [&](const std::string& pkg, int depth, bool isLast) {
            std::string indent;
            for (int i = 0; i < depth - 1; ++i) indent += "│   ";
            if (depth > 0) indent += (isLast ? "└── " : "├── ");

            auto it = graph_.find(pkg);
            std::string ver = (it != graph_.end()) ? it->second.version : "";
            std::string installed_marker = (it != graph_.end() && it->second.installed)
                                           ? " ✓" : "";

            result += indent + pkg +
                      (ver.empty() ? "" : (" " + ver)) +
                      installed_marker + "\n";

            if (depth < maxDepth && it != graph_.end()) {
                auto& deps = it->second.deps;
                for (size_t i = 0; i < deps.size(); ++i) {
                    draw(deps[i], depth + 1, i == deps.size() - 1);
                }
            }
        };

    draw(rootPkg, 0, true);
    return result;
}

// ── 孤包检测 ──────────────────────────────────────────────
std::vector<std::string> DependencyResolver::findOrphans() {
    // 统计每个包被多少包依赖（反向依赖数）
    std::map<std::string, int> refCount;
    for (auto& [name, node] : graph_) {
        if (!refCount.count(name)) refCount[name] = 0;
        for (auto& dep : node.deps) refCount[dep]++;
    }

    std::vector<std::string> orphans;
    for (auto& [name, node] : graph_) {
        if (node.installed && refCount[name] == 0) {
            orphans.push_back(name);
        }
    }
    return orphans;
}

// ── Backend 选择 ──────────────────────────────────────────
std::string DependencyResolver::selectBestBackend(
    const std::string& pkg,
    const std::vector<BackendPriority>& priorities) {

    // 按优先级分数排序
    auto sorted = priorities;
    std::sort(sorted.begin(), sorted.end(),
              [](const BackendPriority& a, const BackendPriority& b) {
                  return a.score > b.score;
              });

    for (auto& p : sorted) {
        // 检查 backend 是否可用（通过 layer manager 查询）
        // 简单实现：返回第一个可用的高优先级 backend
        return p.backend;
    }

    return "";
}

// ── 默认 Backend 配置 ─────────────────────────────────────
std::string PriorityConfig::defaultForDistro(const std::string& distro) {
    static const std::map<std::string, std::string> mapping = {
        {"ubuntu",      "apt"},
        {"debian",      "apt"},
        {"fedora",      "dnf"},
        {"centos",      "dnf"},
        {"rhel",        "dnf"},
        {"arch",        "pacman"},
        {"manjaro",     "pacman"},
        {"opensuse",    "zypper"},
        {"gentoo",      "portage"},
        {"nixos",       "nix"},
        {"freebsd",     "pkg"},
        {"macos",       "brew"},
        {"darwin",      "brew"},
    };
    std::string lower = distro;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = mapping.find(lower);
    return it != mapping.end() ? it->second : "apt";
}

} // namespace SSPM
