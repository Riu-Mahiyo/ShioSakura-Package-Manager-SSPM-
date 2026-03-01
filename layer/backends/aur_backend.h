#pragma once
#include "../backend.h"
#include <string>

namespace SSPM {

// AUR Backend (Arch Linux AUR)
// 优先级检测：yay → paru → makepkg（手动）
// 注意：AUR 需要非 root 运行（makepkg 禁止 root）
class AurBackend : public LayerBackend {
public:
    std::string name() const override { return "aur"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // 检测哪个 AUR helper 可用
    static std::string aurHelper();

private:
    // makepkg 手动安装（无 AUR helper 时的 fallback）
    static BackendResult makepkgInstall(const std::string& pkg);
};

} // namespace SSPM
