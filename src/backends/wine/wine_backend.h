#pragma once
#include "sspm/backend.h"

namespace sspm::backends {

// Wine Backend — 通过 winetricks 管理 Wine 组件
// 支持平台：Linux、macOS（需要安装 wine）
// 用途：安装 Windows 运行库（vcredist、dotnet、directx 等）
class WineBackend : public sspm::Backend {
public:
    std::string name() const override { return "wine"; }
    bool is_available() const override;

    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;

    static bool wineAvailable();
    static bool winetricksAvailable();
    static std::string wineVersion();
};

} // namespace sspm::backends
