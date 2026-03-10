#pragma once
#include "../backend.h"

namespace SSPM {

// dpkg — 直接操作 .deb 文件（低层，apt 的底层）
// 用途：离线安装本地 .deb，查询已安装包信息
// 不同于 apt：不解析依赖，不联网
class DpkgBackend : public LayerBackend {
public:
    std::string name() const override { return "dpkg"; }
    bool available() const override;

    // pkgs 参数：.deb 文件路径 或 包名（用于 remove/query）
    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;
};

} // namespace SSPM
