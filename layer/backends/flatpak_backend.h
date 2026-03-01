#pragma once
#include "../backend.h"

namespace SSPM {

class FlatpakBackend : public LayerBackend {
public:
    std::string name() const override { return "flatpak"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // flatpak 特有：检查 remote 配置
    static bool hasFlathub();
    static bool addFlathub();
};

} // namespace SSPM
