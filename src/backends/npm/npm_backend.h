#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// npm global package manager
class NpmBackend : public sspm::Backend {
public:
    std::string name() const override { return "npm"; }
    bool is_available() const override;

    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;
};

} // namespace sspm::backends
