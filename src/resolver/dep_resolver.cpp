// dep_resolver.cpp
// Public API facade for the dependency resolver subsystem.
// The actual work is delegated to:
//   version_solver.cpp  — VersionConstraint, Dependency, compare_versions
//   dep_graph.cpp       — expand_deps, topological_sort, pick_version
//   conflict_solver.cpp — check_conflicts, resolve_upgrade_all, resolve_remove

#include "sspm/dep_resolver.h"

namespace sspm {

SolveResult DepResolver::resolve_install(const std::string& pkg_name,
                                          const VersionConstraint& constraint) {
    return resolve_install_set({{ pkg_name, constraint }});
}

SolveResult DepResolver::resolve_install_set(
    const std::vector<std::pair<std::string, VersionConstraint>>& requests)
{
    SolverState state;
    bool all_ok = true;

    for (auto& [name, constraint] : requests) {
        if (!expand_deps(name, constraint, "user", state))
            all_ok = false;
    }

    if (!all_ok || !state.errors.empty())
        return { false, {}, state.errors, state.warnings };

    auto plan = topological_sort(state);

    if (!state.errors.empty())
        return { false, {}, state.errors, state.warnings };

    // Tag explicitly-requested packages in the plan
    for (auto& entry : plan) {
        for (auto& [reqname, _] : requests) {
            if (entry.package.name == reqname)
                entry.reason = "requested";
        }
    }

    return { true, plan, state.errors, state.warnings };
}

} // namespace sspm
