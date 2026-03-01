#pragma once
#include "../backend.h"

namespace SSPM {

// ports — FreeBSD Ports Collection
// 平台：仅 FreeBSD（Linux/macOS 禁用）
// 与 pkg 的区别：ports 是从源码编译安装
// 路径：/usr/ports/<category>/<name>
class PortsBackend : public LayerBackend {
public:
    std::string name() const override { return "ports"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

private:
    static bool findPort(const std::string& pkg, std::string& portDir);
};

} // namespace SSPM
