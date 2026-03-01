#pragma once
#include "../backend.h"

namespace SSPM {

// cargo — Rust 包管理器（全局 crate 安装）
// 安装路径：~/.cargo/bin/
// 支持：所有平台（Linux / macOS / FreeBSD / Windows）
class CargoBackend : public LayerBackend {
public:
    std::string name() const override { return "cargo"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

private:
    static std::string cargoBin();  // cargo 可执行路径
};

} // namespace SSPM
