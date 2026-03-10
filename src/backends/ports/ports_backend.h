#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// ports — FreeBSD Ports Collection
// 平台：仅 FreeBSD（Linux/macOS 禁用）
// 与 pkg 的区别：ports 是从源码编译安装
// 路径：/usr/ports/<category>/<name>
class PortsBackend : public sspm::Backend {
public:
    std::string name() const override { return "ports"; }
    bool is_available() const override;

    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;

private:
    static bool findPort(const std::string& pkg, std::string& portDir);
};

} // namespace sspm::backends
