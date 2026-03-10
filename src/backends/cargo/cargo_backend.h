#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// cargo — Rust 包管理器（全局 crate 安装）
// 安装路径：~/.cargo/bin/
// 支持：所有平台（Linux / macOS / FreeBSD / Windows）
class CargoBackend : public sspm::Backend {
public:
    std::string name() const override { return "cargo"; }
    bool is_available() const override;

    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;

private:
    static std::string cargoBin();  // cargo 可执行路径
};

} // namespace sspm::backends
