// conflict_solver.cpp
// Responsible for: package conflict detection, breaks checking, reverse-dep
// lookups, and virtual package (provides) satisfaction.
//
// This file grows when:
//   - "breaks" checking is wired to the installed package database
//   - Virtual package resolution (provides) needs fuzzy matching
//   - Conflict reporting gets interactive resolution suggestions

#include "sspm/dep_resolver.h"
#include <iostream>

namespace sspm {

// Check whether pkg conflicts with anything already selected in this solve.
// Also checks the "breaks" list (pkg declares it will break an already-selected pkg).
bool DepResolver::check_conflicts(const SolvablePackage& pkg,
                                  const SolverState& state,
                                  std::vector<std::string>& errors) const {
    bool ok = true;

    // ── conflicts: pkg cannot coexist with these ──────────────────────────────
    for (auto& conflict : pkg.conflicts) {
        auto it = state.selected.find(conflict.name);
        if (it == state.selected.end()) continue;

        if (conflict.constraint.satisfied_by(it->second.version)) {
            errors.push_back(
                "Conflict: installing " + pkg.name + " " + pkg.version +
                " conflicts with " + conflict.name + " " +
                conflict.constraint.to_string() +
                " (in plan: " + it->second.version + ")"
            );
            ok = false;
        }
    }

    // ── breaks: pkg declares it breaks these already-selected packages ────────
    for (auto& brk : pkg.breaks) {
        auto it = state.selected.find(brk.name);
        if (it == state.selected.end()) continue;

        if (brk.constraint.satisfied_by(it->second.version)) {
            errors.push_back(
                "Breaks: " + pkg.name + " " + pkg.version +
                " breaks " + brk.name + " " +
                brk.constraint.to_string() +
                " (in plan: " + it->second.version + ")"
            );
            ok = false;
        }
    }

    // ── reverse conflict check: does anything already selected conflict with us?
    for (auto& [sel_name, sel_pkg] : state.selected) {
        for (auto& existing_conflict : sel_pkg.conflicts) {
            if (existing_conflict.name != pkg.name) continue;
            if (existing_conflict.constraint.satisfied_by(pkg.version)) {
                errors.push_back(
                    "Conflict: " + sel_name + " (already in plan) conflicts with " +
                    pkg.name + " " + existing_conflict.constraint.to_string()
                );
                ok = false;
            }
        }
    }

    return ok;
}

// Resolve upgrade plan: diff installed vs latest in index.
// For each installed package, if index has a newer version → mark for upgrade.
SolveResult DepResolver::resolve_upgrade_all(const std::vector<Package>& installed) {
    SolveResult result;
    result.success = true;

    int upgradeable = 0;
    for (auto& inst : installed) {
        auto latest = index_.find(inst.name);
        if (!latest) {
            result.warnings.push_back(
                inst.name + " not found in index — skipping"
            );
            continue;
        }

        if (compare_versions(latest->version, inst.version) > 0) {
            SolvablePackage upgrade_pkg;
            upgrade_pkg.name    = latest->name;
            upgrade_pkg.version = latest->version;
            upgrade_pkg.backend = latest->backend;
            result.plan.push_back({
                PlanAction::Upgrade,
                upgrade_pkg,
                "upgrade: " + inst.version + " → " + latest->version
            });
            upgradeable++;
        } else {
            result.plan.push_back({
                PlanAction::Keep,
                { inst.name, inst.version, inst.backend, {}, {}, {} },
                "already up to date"
            });
        }
    }

    if (upgradeable == 0)
        result.warnings.push_back("All packages are up to date.");
    else
        std::cout << "[resolver] " << upgradeable << " package(s) can be upgraded\n";

    return result;
}

// Reverse-dependency aware remove.
// Finds packages in `installed` that depend on `pkg_name` and warns/blocks.
SolveResult DepResolver::resolve_remove(const std::string& pkg_name,
                                         const std::vector<Package>& installed) {
    SolveResult result;
    result.success = true;

    // Future: query SkyDB dep_cache table for installed rdeps
    // For now: placeholder that correctly identifies the requested removal
    result.plan.push_back({
        PlanAction::Remove,
        { pkg_name, "", "", {}, {}, {} },
        "requested"
    });

    return result;
}

} // namespace sspm
