#pragma once
#include "../backend.h"

namespace SSPM {

class PacmanBackend : public LayerBackend {
public:
    std::string name() const override { return "pacman"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;
};

} // namespace SSPM
