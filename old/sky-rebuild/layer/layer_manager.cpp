// layer/layer_manager.cpp — SSPM Backend 注册中心（v3.0 完整版）
// 支持 25 个 backend
#include "layer_manager.h"
// ── Linux 原生 ──────────────────────────────────────────
#include "backends/apt_backend.h"
#include "backends/dpkg_backend.h"
#include "backends/pacman_backend.h"
#include "backends/aur_backend.h"
#include "backends/dnf_backend.h"
#include "backends/zypper_backend.h"
#include "backends/portage_backend.h"
// ── macOS ───────────────────────────────────────────────
#include "backends/brew_backend.h"
#include "backends/macports_backend.h"
// ── BSD ─────────────────────────────────────────────────
#include "backends/pkg_backend.h"
#include "backends/ports_backend.h"
// ── 通用 / 跨平台 ───────────────────────────────────────
#include "backends/flatpak_backend.h"
#include "backends/snap_backend.h"
#include "backends/nix_backend.h"
#include "backends/npm_backend.h"
#include "backends/pip_backend.h"
#include "backends/cargo_backend.h"
#include "backends/gem_backend.h"
#include "backends/portable_backend.h"
#include "backends/wine_backend.h"
// ── Sky/SSPM 自有格式 ───────────────────────────────────
#include "backends/spk_backend.h"
#include "backends/amber_backend.h"
// ────────────────────────────────────────────────────────
#include "../core/detect.h"

namespace SSPM {

std::map<std::string, std::unique_ptr<LayerBackend>> LayerManager::registry_;

void LayerManager::registerAll() {
    auto reg = [](std::unique_ptr<LayerBackend> b) {
        auto name = b->name();
        registry_[name] = std::move(b);
    };

    // ── Linux 原生 ────────────────────────────────────────
    reg(std::make_unique<AptBackend>());       // Debian/Ubuntu/Mint
    reg(std::make_unique<DpkgBackend>());      // .deb 直接安装
    reg(std::make_unique<PacmanBackend>());    // Arch Linux
    reg(std::make_unique<AurBackend>());       // AUR (yay/paru/makepkg)
    reg(std::make_unique<DnfBackend>());       // Fedora/RHEL/CentOS
    reg(std::make_unique<ZypperBackend>());    // openSUSE
    reg(std::make_unique<PortageBackend>());   // Gentoo

    // ── macOS ─────────────────────────────────────────────
    reg(std::make_unique<BrewBackend>());      // Homebrew / Linuxbrew
    reg(std::make_unique<MacPortsBackend>());  // MacPorts (支持老 macOS)

    // ── BSD ───────────────────────────────────────────────
    reg(std::make_unique<PkgBackend>());       // FreeBSD pkg (二进制)
    reg(std::make_unique<PortsBackend>());     // FreeBSD Ports (源码)

    // ── 通用 / 跨平台 ─────────────────────────────────────
    reg(std::make_unique<FlatpakBackend>());   // Flatpak (Linux)
    reg(std::make_unique<SnapBackend>());      // Snap (Linux/macOS)
    reg(std::make_unique<NixBackend>());       // Nix
    reg(std::make_unique<NpmBackend>());       // npm global
    reg(std::make_unique<PipBackend>());       // pip/pip3
    reg(std::make_unique<CargoBackend>());     // Rust cargo
    reg(std::make_unique<GemBackend>());       // Ruby gem
    reg(std::make_unique<PortableBackend>());  // AppImage
    reg(std::make_unique<WineBackend>());      // Wine/winetricks

    // ── SSPM 自有格式 ──────────────────────────────────────
    reg(std::make_unique<SpkBackend>());       // .spk 包格式
    reg(std::make_unique<AmberBackend>());     // Amber PM
}

LayerBackend* LayerManager::get(const std::string& name) {
    auto it = registry_.find(name);
    if (it == registry_.end()) return nullptr;
    return it->second.get();  // 不过滤 available()，由调用方决定
}

LayerBackend* LayerManager::getAvailable(const std::string& name) {
    auto* be = get(name);
    if (!be || !be->available()) return nullptr;
    return be;
}

LayerBackend* LayerManager::getDefault() {
    return getAvailable(Detect::defaultBackend());
}

std::vector<std::string> LayerManager::availableNames() {
    std::vector<std::string> result;
    for (auto& [name, backend] : registry_) {
        if (backend->available()) result.push_back(name);
    }
    return result;
}

std::vector<std::string> LayerManager::allNames() {
    std::vector<std::string> result;
    for (auto& [name, _] : registry_) result.push_back(name);
    return result;
}

} // namespace SSPM
