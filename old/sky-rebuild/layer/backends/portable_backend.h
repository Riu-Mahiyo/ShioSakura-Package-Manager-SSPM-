#pragma once
#include "../backend.h"

namespace SSPM {

// Portable Backend — AppImage 管理
// 功能：
//   install：下载 .AppImage 到 ~/.local/share/sspm/portable/<pkg>/
//            chmod +x，创建符号链接到 ~/.local/bin/
//            可选：集成 .desktop 文件（调用桌面集成模块）
//   remove：删除文件和符号链接
//   list：显示已安装的 AppImage
//
// 包名规范：可以是 URL 或注册表中的包名
class PortableBackend : public LayerBackend {
public:
    std::string name() const override { return "portable"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    static std::string portableDir();
    static std::string binDir();

private:
    static BackendResult installFromUrl(const std::string& url,
                                         const std::string& name);
    static BackendResult installFromFile(const std::string& path,
                                          const std::string& name);
};

} // namespace SSPM
