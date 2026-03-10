#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// pip global/user packages
class PipBackend : public sspm::Backend {
public:
    std::string name() const override { return "pip"; }
    bool is_available() const override;

    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;

private:
    // 优先用 pip3，兜底用 pip
    static std::string pipCmd();
};

} // namespace sspm::backends
