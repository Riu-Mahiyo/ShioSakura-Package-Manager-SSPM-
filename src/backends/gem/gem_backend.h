#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// gem — Ruby 包管理器
// 支持：Linux / macOS / FreeBSD
// 注意：--user-install 避免 root 安装
class GemBackend : public sspm::Backend {
public:
    std::string name() const override { return "gem"; }
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
