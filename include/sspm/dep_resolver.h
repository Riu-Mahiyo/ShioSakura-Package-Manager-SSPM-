#pragma once
#include "sspm/backend.h"
#include "sspm/index.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <variant>

namespace sspm {

// ── Version constraint types ──────────────────────────────────────────────────
// Represents a version requirement like ">=1.2.0", "=2.0", "<3"
enum class VersionOp { Any, Eq, Gte, Gt, Lte, Lt, Ne };

struct VersionConstraint {
    VersionOp op = VersionOp::Any;
    std::string version;           // e.g. "1.25.0"

    static VersionConstraint parse(const std::string& expr);
    bool satisfied_by(const std::string& candidate) const;
    std::string to_string() const;
};

// A dependency requirement: package name + optional version constraint
struct Dependency {
    std::string name;
    VersionConstraint constraint;
    bool optional = false;

    static Dependency parse(const std::string& expr); // "libc >= 2.17"
};

// A conflict declaration: this package must NOT be installed alongside target
struct Conflict {
    std::string name;
    VersionConstraint constraint;
};

// Full package metadata used by the solver
struct SolvablePackage {
    std::string name;
    std::string version;
    std::string backend;
    std::vector<Dependency> depends;
    std::vector<Dependency> provides;  // virtual packages this satisfies
    std::vector<Conflict>   conflicts;
    std::vector<Conflict>   breaks;
};

// ── Install plan ──────────────────────────────────────────────────────────────
enum class PlanAction { Install, Upgrade, Remove, Keep };

struct PlanEntry {
    PlanAction action;
    SolvablePackage package;
    std::string reason;   // e.g. "requested", "dependency of nginx", "conflict resolution"
};

struct SolveResult {
    bool success;
    std::vector<PlanEntry> plan;      // ordered install plan (topological)
    std::vector<std::string> errors;  // conflict/unsatisfied dep messages
    std::vector<std::string> warnings;
};

// ── Dependency Resolver ───────────────────────────────────────────────────────
//
// Algorithm (CDCL-lite, similar to apt's resolver):
//
//  1. For each requested package, find best matching version in index
//  2. Recursively expand all dependencies
//  3. Detect version conflicts: two requirements need incompatible versions
//  4. Detect package conflicts: explicit conflict declarations
//  5. Topological sort to produce install order
//  6. On conflict: backtrack and try alternative versions
//
// This is intentionally a simplified but correct solver.
// A full SAT-based solver (like apt's CUDF) is tracked in future work.
class DepResolver {
public:
    explicit DepResolver(const PackageIndex& index);

    // Resolve a single install request
    SolveResult resolve_install(const std::string& pkg_name,
                                const VersionConstraint& constraint = {});

    // Resolve multiple simultaneous installs (handles inter-package conflicts)
    SolveResult resolve_install_set(
        const std::vector<std::pair<std::string, VersionConstraint>>& requests);

    // Resolve a remove request (checks reverse dependencies)
    SolveResult resolve_remove(const std::string& pkg_name,
                               const std::vector<Package>& installed);

    // Resolve upgrade-all: diff installed vs available, produce upgrade plan
    SolveResult resolve_upgrade_all(const std::vector<Package>& installed);

private:
    const PackageIndex& index_;

    // Internal solver state
    struct SolverState {
        std::unordered_map<std::string, SolvablePackage> selected;   // name → chosen version
        std::unordered_map<std::string, std::vector<VersionConstraint>> requirements; // accumulated constraints
        std::unordered_set<std::string> visited;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    bool expand_deps(const std::string& name,
                     const VersionConstraint& constraint,
                     const std::string& requested_by,
                     SolverState& state);

    std::optional<SolvablePackage> pick_version(
        const std::string& name,
        const std::vector<VersionConstraint>& constraints) const;

    bool check_conflicts(const SolvablePackage& pkg,
                         const SolverState& state,
                         std::vector<std::string>& errors) const;

    std::vector<PlanEntry> topological_sort(SolverState& state) const;

    static SolvablePackage from_index_entry(const IndexEntry& e);

public:
    static int compare_versions(const std::string& a, const std::string& b);
};

} // namespace sspm
