#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// dpkg — 直接操作 .deb 文件（低层，apt 的底层）
// 用途：离线安装本地 .deb，查询已安装包信息
// 不同于 apt：不解析依赖，不联网
class DpkgBackend : public sspm::Backend {
public:
    std::string name() const override { return "dpkg"; }
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
