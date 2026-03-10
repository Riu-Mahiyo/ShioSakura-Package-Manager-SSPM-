#pragma once
#include "../backend.h"

namespace SSPM {

// Nix 包管理器（Linux + macOS）
// 参考 v1.9 nix.sh 的路径检测和 PATH 修复逻辑
class NixBackend : public LayerBackend {
public:
    std::string name() const override { return "nix"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // Nix 特有：PATH 修复（nix-profile 路径注入）
    static std::string nixProfileBin();
    static bool        fixPath();
    static bool        daemonRunning();
};

} // namespace SSPM
