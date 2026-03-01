#pragma once
#include "../backend.h"

namespace SSPM {

class SnapBackend : public LayerBackend {
public:
    std::string name() const override { return "snap"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // snap 特有：检查 snapd 是否运行
    static bool snapdRunning();
};

} // namespace SSPM
