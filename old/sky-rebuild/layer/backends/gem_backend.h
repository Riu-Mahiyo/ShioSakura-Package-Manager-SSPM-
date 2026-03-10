#pragma once
#include "../backend.h"

namespace SSPM {

// gem — Ruby 包管理器
// 支持：Linux / macOS / FreeBSD
// 注意：--user-install 避免 root 安装
class GemBackend : public LayerBackend {
public:
    std::string name() const override { return "gem"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;
};

} // namespace SSPM
