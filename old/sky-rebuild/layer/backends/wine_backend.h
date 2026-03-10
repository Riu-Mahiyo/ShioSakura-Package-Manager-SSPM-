#pragma once
#include "../backend.h"

namespace SSPM {

// Wine Backend — 通过 winetricks 管理 Wine 组件
// 支持平台：Linux、macOS（需要安装 wine）
// 用途：安装 Windows 运行库（vcredist、dotnet、directx 等）
class WineBackend : public LayerBackend {
public:
    std::string name() const override { return "wine"; }
    bool available() const override;

    // pkgs = winetricks verb，如 "vcredist2019" "dotnet48" "d3dx9"
    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    static bool wineAvailable();
    static bool winetricksAvailable();
    static std::string wineVersion();
};

} // namespace SSPM
