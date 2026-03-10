#pragma once
#include "backend.h"
#include <vector>
#include <memory>
#include <string>

namespace sspm {

// Selects the best backend for a given package
// based on: user priority config → availability → fallback
class Resolver {
public:
    explicit Resolver(std::vector<std::shared_ptr<Backend>> backends);

    // Returns the best backend for installing `pkg_name`
    std::shared_ptr<Backend> resolve(const std::string& pkg_name) const;

    // Search across all backends and merge results
    SearchResult search_all(const std::string& query) const;

    void set_priority(const std::vector<std::string>& order);

private:
    std::vector<std::shared_ptr<Backend>> backends_;
    std::vector<std::string> priority_;
};

} // namespace sspm
