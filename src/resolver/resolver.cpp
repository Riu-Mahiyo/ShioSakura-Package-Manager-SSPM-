#include "sspm/resolver.h"
#include <algorithm>
#include <iostream>

namespace sspm {

Resolver::Resolver(std::vector<std::shared_ptr<Backend>> backends)
    : backends_(std::move(backends)) {}

void Resolver::set_priority(const std::vector<std::string>& order) {
    priority_ = order;
}

// Returns the highest-priority backend that:
//   1. Is available on the current system
//   2. Has the requested package in its index
std::shared_ptr<Backend> Resolver::resolve(const std::string& pkg_name) const {
    // Build ordered list: priority-listed backends first, then remaining
    std::vector<std::shared_ptr<Backend>> ordered;
    for (auto& name : priority_) {
        for (auto& b : backends_) {
            if (b->name() == name && b->is_available())
                ordered.push_back(b);
        }
    }
    for (auto& b : backends_) {
        bool already = false;
        for (auto& o : ordered) if (o->name() == b->name()) { already = true; break; }
        if (!already && b->is_available()) ordered.push_back(b);
    }

    for (auto& b : ordered) {
        auto result = b->search(pkg_name);
        for (auto& pkg : result.packages) {
            if (pkg.name == pkg_name) {
                std::cout << "[resolver] Selected backend: " << b->name() << "\n";
                return b;
            }
        }
    }

    std::cerr << "[resolver] No backend has package: " << pkg_name << "\n";
    return nullptr;
}

// Search across ALL available backends and merge results
SearchResult Resolver::search_all(const std::string& query) const {
    SearchResult merged;
    merged.success = true;
    for (auto& b : backends_) {
        if (!b->is_available()) continue;
        auto r = b->search(query);
        if (!r.success) {
            merged.success = false;
            // Optionally merge error messages
            continue;
        }
        for (auto& pkg : r.packages) {
            pkg.backend = b->name();
            merged.packages.push_back(pkg);
        }
    }
    return merged;
}

} // namespace sspm
