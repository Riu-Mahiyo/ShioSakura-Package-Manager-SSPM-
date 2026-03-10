// dep_graph.cpp
// Responsible for: recursive dependency graph expansion (DFS) and
// Kahn's-algorithm topological sort to produce an ordered install plan.
//
// This file grows when:
//   - Optional dependency handling gets more nuanced
//   - Parallel dependency expansion is added (future)
//   - Virtual package resolution is added (provides → satisfies dep)

#include "sspm/dep_resolver.h"
#include <iostream>

namespace sspm {

DepResolver::DepResolver(const PackageIndex& index) : index_(index) {}

SolvablePackage DepResolver::from_index_entry(const IndexEntry& e) {
    SolvablePackage p;
    p.name    = e.name;
    p.version = e.version;
    p.backend = e.backend;
    for (auto& dep : e.depends) p.depends.push_back(Dependency::parse(dep));
    return p;
}

// Pick the candidate version that satisfies ALL accumulated constraints.
// When the index supports multiple versions per package (future), this will
// try versions from newest to oldest and return the first satisfying one.
std::optional<SolvablePackage> DepResolver::pick_version(
    const std::string& name,
    const std::vector<VersionConstraint>& constraints) const
{
    auto entry = index_.find(name);
    if (!entry) return std::nullopt;

    SolvablePackage candidate = from_index_entry(*entry);
    for (auto& c : constraints) {
        if (!c.satisfied_by(candidate.version)) return std::nullopt;
    }
    return candidate;
}

// Recursive DFS dependency expansion.
// Design invariants:
//   - visited set prevents infinite loops on cycles (A → B → A)
//   - Constraints are accumulated before pick_version so ANDing is correct
//   - Optional deps are recorded but not expanded in the base pass
bool DepResolver::expand_deps(const std::string& name,
                               const VersionConstraint& constraint,
                               const std::string& requested_by,
                               SolverState& state) {
    // Guard against cycles (e.g. lib-a → lib-b → lib-a)
    if (state.visited.count(name)) return true;
    state.visited.insert(name);

    // Accumulate new constraint
    state.requirements[name].push_back(constraint);

    // Already selected — just verify the new constraint is compatible
    if (state.selected.count(name)) {
        auto& sel = state.selected[name];
        if (!constraint.satisfied_by(sel.version)) {
            state.errors.push_back(
                "Version conflict: " + name +
                " " + constraint.to_string() +
                " required by " + requested_by +
                ", but " + sel.version + " already selected"
            );
            return false;
        }
        state.visited.erase(name);
        return true;
    }

    // Pick best version satisfying all accumulated constraints
    auto pkg = pick_version(name, state.requirements[name]);
    if (!pkg) {
        bool has_specific_constraint =
            !constraint.version.empty() ||
            state.requirements[name].size() > 1;

        if (has_specific_constraint) {
            state.errors.push_back(
                "Unsatisfied dependency: " + name +
                " " + constraint.to_string() +
                " (required by " + requested_by + ")"
            );
        } else {
            // Package not in index — warn but continue (backend may have it)
            state.warnings.push_back(
                "Package not in index: " + name +
                " (required by " + requested_by +
                ") — will probe backends at install time"
            );
            SolvablePackage placeholder;
            placeholder.name    = name;
            placeholder.version = constraint.version.empty() ? "any" : constraint.version;
            placeholder.backend = "auto";
            state.selected[name] = placeholder;
        }
        state.visited.erase(name);
        return true;
    }

    // Check for conflicts with already-selected packages
    if (!check_conflicts(*pkg, state, state.errors)) {
        state.visited.erase(name);
        return false;
    }

    state.selected[name] = *pkg;

    // Recurse into required dependencies
    for (auto& dep : pkg->depends) {
        if (dep.optional) {
            state.warnings.push_back(
                "Optional dep skipped: " + dep.name +
                " (use --with-recommends to include)"
            );
            continue;
        }
        bool ok = expand_deps(dep.name, dep.constraint, name, state);
        if (!ok) {
            state.visited.erase(name);
            return false;
        }
    }

    state.visited.erase(name);
    return true;
}

// Kahn's algorithm: produce a topologically ordered install plan.
// Packages with no unresolved dependencies are installed first.
std::vector<PlanEntry> DepResolver::topological_sort(SolverState& state) const {
    // Build adjacency list and in-degree map
    std::unordered_map<std::string, std::vector<std::string>> dependents_of;
    std::unordered_map<std::string, int> in_degree;

    for (auto& [name, _] : state.selected) in_degree[name] = 0;

    for (auto& [name, pkg] : state.selected) {
        for (auto& dep : pkg.depends) {
            if (!state.selected.count(dep.name)) continue;
            dependents_of[dep.name].push_back(name);
            in_degree[name]++;
        }
    }

    // Seed: packages with zero in-degree (no deps in the install set)
    std::vector<std::string> ready;
    for (auto& [name, deg] : in_degree)
        if (deg == 0) ready.push_back(name);

    std::vector<PlanEntry> plan;
    while (!ready.empty()) {
        std::string cur = ready.back();
        ready.pop_back();

        plan.push_back({
            PlanAction::Install,
            state.selected.at(cur),
            "dependency"   // overridden by public API for requested packages
        });

        for (auto& dep_name : dependents_of[cur]) {
            if (--in_degree[dep_name] == 0)
                ready.push_back(dep_name);
        }
    }

    // Check for cycles: if plan size != selected size, we have a cycle
    if (plan.size() < state.selected.size()) {
        std::string cycle_pkgs;
        for (auto& [name, deg] : in_degree) {
            if (deg > 0) {
                if (!cycle_pkgs.empty()) cycle_pkgs += ", ";
                cycle_pkgs += name;
            }
        }
        state.errors.push_back("Circular dependency detected among: " + cycle_pkgs);
    }

    return plan;
}

} // namespace sspm
