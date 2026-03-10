#pragma once
#include "sspm/backend.h"
#include <string>

namespace sspm::backends {

// AUR Backend (Arch Linux AUR)
// 优先级检测：yay → paru → makepkg（手动）
// 注意：AUR 需要非 root 运行（makepkg 禁止 root）
class AurBackend : public sspm::Backend {
public:
    std::string name() const override { return "aur"; }
    bool is_available() const override;

    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;

    // 检测哪个 AUR helper 可用
    static std::string aurHelper();

private:
    // makepkg 手动安装（无 AUR helper 时的 fallback）
    static sspm::Result makepkgInstall(const std::string& pkg);
};

} // namespace sspm::backends
