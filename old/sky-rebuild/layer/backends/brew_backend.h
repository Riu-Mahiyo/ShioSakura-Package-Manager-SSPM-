#pragma once
#include "../backend.h"

namespace SSPM {

// Homebrew (macOS) + Linuxbrew (Linux)
// 参考 v1.9 brew.sh 的路径检测逻辑
class BrewBackend : public LayerBackend {
public:
    std::string name() const override { return "brew"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // brew 特有：PATH 修复（Linuxbrew 场景）
    static std::string brewPrefix();
    static bool        fixPath();

private:
    static std::string findBrew();
};

} // namespace SSPM
