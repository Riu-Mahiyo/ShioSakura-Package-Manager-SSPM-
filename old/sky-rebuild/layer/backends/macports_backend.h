#pragma once
#include "../backend.h"

namespace SSPM {

// MacPorts — macOS 包管理器（Homebrew 的替代品）
// 适用于：旧版 macOS（≤10.14）、科研/服务器场景
class MacPortsBackend : public LayerBackend {
public:
    std::string name() const override { return "macports"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;
};

} // namespace SSPM
