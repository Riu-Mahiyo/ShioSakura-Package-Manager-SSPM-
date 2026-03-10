#pragma once
#include "../backend.h"

namespace SSPM {

// pip global/user packages
class PipBackend : public LayerBackend {
public:
    std::string name() const override { return "pip"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

private:
    // 优先用 pip3，兜底用 pip
    static std::string pipCmd();
};

} // namespace SSPM
