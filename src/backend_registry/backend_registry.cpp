#include "sspm/backend_registry.h"
#include "sspm/backend.h"
#include "sspm/log.h"
#include "sspm/plugin.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <future>
#include <map>
#include <unistd.h>

namespace fs = std::filesystem;

namespace sspm {

BackendRegistry::BackendRegistry() = default;

static std::map<std::string, BackendRegistry::FactoryFn>& global_factories() {
    static std::map<std::string, BackendRegistry::FactoryFn> factories;
    return factories;
}

void BackendRegistry::register_factory(const std::string& name, FactoryFn factory) {
    global_factories()[name] = std::move(factory);
}

void BackendRegistry::register_from_plugins(const plugin::PluginManager& manager) {
    auto instances = manager.get_backend_plugins();
    for (auto& instance : instances) {
        register_instance(instance);
    }
}

void BackendRegistry::register_instance(std::shared_ptr<Backend> instance) {
    if (!instance) return;
    
    // Check if already registered
    for (auto& e : entries_) {
        if (e.instance == instance) return;
    }

    BackendDescriptor desc;
    desc.name = instance->name();
    desc.platform = "universal";
    desc.family = "plugin";
    desc.default_priority = 60; // plugins have lower priority by default
    
    entries_.push_back({ std::move(desc), instance, true, true });
    cache_valid_ = false;
}

// Helper macro to create a factory lambda for a backend class
#define BACKEND_FACTORY(name) \
    []() -> std::shared_ptr<sspm::Backend> { \
        auto it = global_factories().find(name); \
        if (it != global_factories().end()) return it->second(); \
        return nullptr; \
    }

void BackendRegistry::load_plugins(const std::string& plugin_dir) {
    if (!fs::exists(plugin_dir)) return;

    for (auto& entry : fs::directory_iterator(plugin_dir)) {
        if (entry.path().extension() == ".so" || entry.path().extension() == ".dylib") {
            // Future: integrate with PluginManager to load shared library backends
            SSPM_DEBUG("found potential backend plugin: " + entry.path().string());
        }
    }
}

bool BackendRegistry::binary_exists(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return fs::exists(path, ec) && !ec;
}

// Test hooks allow us to artificially flip the detection logic during unit
// tests.  Only the Alpine override is needed today but we keep a second
// variable in case future tests exercise more exotic musl-only scenarios.
static bool s_test_force_alpine = false;
static bool s_test_force_musl   = false;

bool BackendRegistry::is_musl() {
    if (s_test_force_musl) {
        SSPM_DEBUG("is_musl() overridden for test");
        return true;
    }
    // treats Alpine as musl as well
    if (s_test_force_alpine) return true;

#ifdef SSPM_LINUX
    // Method 1: Check common musl dynamic linker paths
    if (binary_exists("/lib/ld-musl-x86_64.so.1") || 
        binary_exists("/lib/ld-musl-aarch64.so.1") ||
        binary_exists("/lib/ld-musl-armhf.so.1") ||
        binary_exists("/lib/ld-musl-i386.so.1")) {
        return true;
    }
    // Method 2: check for the absence of glibc specific paths
    if (!binary_exists("/lib/ld-linux-x86-64.so.2") && 
        !binary_exists("/etc/ld.so.conf")) {
        // This is a weak hint, better combine with alpine check
        return is_alpine();
    }
#endif
    return false;
}

bool BackendRegistry::is_alpine() {
    if (s_test_force_alpine) {
        SSPM_DEBUG("is_alpine() overridden for test");
        return true;
    }
#ifdef SSPM_LINUX
    return binary_exists("/etc/alpine-release");
#endif
    return false;
}

// test helper implementations
void BackendRegistry::set_test_force_alpine(bool v) {
    s_test_force_alpine = v;
}

void BackendRegistry::set_test_force_musl(bool v) {
    s_test_force_musl = v;
}


bool BackendRegistry::probe(Entry& e) {
    if (e.probed) return e.available;
    e.probed = true;

    // Special case for APK: only on Alpine proper.  We previously allowed any
    // musl-based distro but that misbehaved on e.g. Alpine derivatives and
    // other musl systems that do not ship apk.
    if (e.desc.name == "apk") {
        if (!is_alpine()) {
            SSPM_DEBUG("apk backend skipped: not an Alpine system");
            return false;
        }
    }

    bool found = binary_exists(e.desc.probe_binary) ||
                 (!e.desc.probe_fallback.empty() &&
                  binary_exists(e.desc.probe_fallback));

    e.available = found;
    if (found) {
        e.instance = e.desc.factory();
        SSPM_DEBUG("backend detected: " + e.desc.name + " (" + e.desc.probe_binary + ")");
    }
    return found;
}

void BackendRegistry::register_backend(BackendDescriptor desc) {
    entries_.push_back({ std::move(desc), nullptr, false, false });
    cache_valid_ = false;
}

std::vector<std::shared_ptr<Backend>> BackendRegistry::available_backends() {
    if (cache_valid_) {
        std::vector<std::shared_ptr<Backend>> result;
        for (auto& e : entries_) {
            if (e.available && e.instance) result.push_back(e.instance);
        }
        return result;
    }

    std::vector<Entry*> available_entries;
    for (auto& e : entries_) {
        if (probe(e) && e.instance) available_entries.push_back(&e);
    }

    // Sort entries_ in place based on priority
    std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        return a.desc.default_priority < b.desc.default_priority;
    });

    cache_valid_ = true;
    
    std::vector<std::shared_ptr<Backend>> result;
    for (auto& e : entries_) {
        if (e.available && e.instance) result.push_back(e.instance);
    }
    return result;
}

std::vector<std::shared_ptr<Backend>> BackendRegistry::all_backends() {
    std::vector<std::shared_ptr<Backend>> result;
    for (auto& e : entries_) {
        if (!e.instance) e.instance = e.desc.factory();
        result.push_back(e.instance);
    }
    return result;
}

std::shared_ptr<Backend> BackendRegistry::get(const std::string& name) {
    for (auto& e : entries_) {
        if (e.desc.name == name) {
            probe(e);
            return e.instance;
        }
    }
    return nullptr;
}

void BackendRegistry::refresh() {
    for (auto& e : entries_) { e.probed = false; e.available = false; }
    cache_valid_ = false;
    SSPM_INFO("backend registry: re-probing all backends");
}

void BackendRegistry::probe_all_async() {
    std::vector<std::future<void>> futures;
    for (auto& e : entries_) {
        if (!e.probed) {
            futures.push_back(std::async(std::launch::async, [this, &e]() {
                this->probe(e);
            }));
        }
    }
    for (auto& f : futures) f.wait();
    cache_valid_ = true;
}

void BackendRegistry::print_detection_report() const {
    std::cout << "\n=== Backend Detection Report ===\n";
    std::cout << std::left;
    for (auto& e : entries_) {
        std::string status;
        if (!e.probed)      status = "not probed";
        else if (e.available) status = "✅ available";
        else                  status = "⬜ not found  (" + e.desc.probe_binary + ")";

        std::cout.width(14); std::cout << e.desc.name;
        std::cout.width(12); std::cout << e.desc.platform;
        std::cout << status << "\n";
    }
    std::cout << "================================\n\n";
}

// ── Static factory: registers all 20 backends ────────────────────────────────
BackendRegistry BackendRegistry::create() {
    BackendRegistry reg;

    // ── Linux — Debian/Ubuntu ─────────────────────────────────────────────────
    reg.register_backend({
        "apt", "/usr/bin/apt-get", "/bin/apt-get",
        "linux", "debian", 10,
        BACKEND_FACTORY("apt")
    });

    // ── Linux — Arch ──────────────────────────────────────────────────────────
    reg.register_backend({
        "pacman", "/usr/bin/pacman", "",
        "linux", "arch", 10,
        BACKEND_FACTORY("pacman")
    });

    // ── Linux — Red Hat/Fedora ────────────────────────────────────────────────
    reg.register_backend({
        "dnf", "/usr/bin/dnf", "/bin/dnf",
        "linux", "redhat", 10,
        BACKEND_FACTORY("dnf")
    });

    // ── Linux — SUSE ──────────────────────────────────────────────────────────
    reg.register_backend({
        "zypper", "/usr/bin/zypper", "",
        "linux", "suse", 10,
        BACKEND_FACTORY("zypper")
    });

    // ── Linux — Gentoo ────────────────────────────────────────────────────────
    reg.register_backend({
        "portage", "/usr/bin/emerge", "",
        "linux", "gentoo", 10,
        BACKEND_FACTORY("portage")
    });

    // ── Linux — Alpine ────────────────────────────────────────────────────────
    reg.register_backend({
        "apk", "/sbin/apk", "/usr/sbin/apk",
        "linux", "alpine", 10,
        BACKEND_FACTORY("apk")
    });

    // ── Linux — Void ──────────────────────────────────────────────────────────
    reg.register_backend({
        "xbps", "/usr/bin/xbps-install", "",
        "linux", "void", 10,
        BACKEND_FACTORY("xbps")
    });

    // ── Linux — NixOS ─────────────────────────────────────────────────────────
    reg.register_backend({
        "nix", "/usr/bin/nix-env", "/run/current-system/sw/bin/nix-env",
        "linux", "nixos", 20,   // lower priority than native PM
        BACKEND_FACTORY("nix")
    });

    // ── Linux — Debian/Ubuntu (dpkg) ──────────────────────────────────────────
    reg.register_backend({
        "dpkg", "/usr/bin/dpkg", "/bin/dpkg",
        "linux", "debian", 15,
        BACKEND_FACTORY("dpkg")
    });

    // ── Linux — Arch (AUR) ────────────────────────────────────────────────────
    reg.register_backend({
        "aur", "/usr/bin/yay", "/usr/bin/paru",
        "linux", "arch", 15,
        BACKEND_FACTORY("aur")
    });

    // ── BSD — FreeBSD ─────────────────────────────────────────────────────────
    reg.register_backend({
        "pkg", "/usr/sbin/pkg", "/usr/local/sbin/pkg",
        "bsd", "freebsd", 10,
        BACKEND_FACTORY("pkg")
    });

    // ── BSD — NetBSD ──────────────────────────────────────────────────────────
    reg.register_backend({
        "pkgin", "/usr/pkg/bin/pkgin", "",
        "bsd", "netbsd", 10,
        BACKEND_FACTORY("pkgin")
    });

    // ── BSD — FreeBSD (Ports) ─────────────────────────────────────────────────
    reg.register_backend({
        "ports", "/usr/ports", "",
        "bsd", "freebsd", 15,
        BACKEND_FACTORY("ports")
    });

    // ── macOS/Linux — Homebrew ───────────────────────────────────────────────
    reg.register_backend({
        "brew", "/usr/local/bin/brew", "/home/linuxbrew/.linuxbrew/bin/brew",
        "universal", "homebrew", 25,
        BACKEND_FACTORY("brew")
    });

    // ── macOS — MacPorts ──────────────────────────────────────────────────────
    reg.register_backend({
        "macports", "/opt/local/bin/port", "",
        "macos", "macports", 20,
        BACKEND_FACTORY("macports")
    });

    // ── Windows — winget ──────────────────────────────────────────────────────
    reg.register_backend({
        "winget", "C:/Users/Default/AppData/Local/Microsoft/WindowsApps/winget.exe", "",
        "windows", "winget", 10,
        BACKEND_FACTORY("winget")
    });

    // ── Windows — Scoop ───────────────────────────────────────────────────────
    reg.register_backend({
        "scoop", "C:/Users/Default/scoop/shims/scoop.cmd", "",
        "windows", "scoop", 15,
        BACKEND_FACTORY("scoop")
    });

    // ── Windows — Chocolatey ──────────────────────────────────────────────────
    reg.register_backend({
        "chocolatey", "C:/ProgramData/chocolatey/bin/choco.exe", "",
        "windows", "chocolatey", 20,
        BACKEND_FACTORY("chocolatey")
    });

    // ── Universal — Flatpak ───────────────────────────────────────────────────
    reg.register_backend({
        "flatpak", "/usr/bin/flatpak", "/usr/local/bin/flatpak",
        "universal", "flatpak", 30,  // universal backends are lower priority
        BACKEND_FACTORY("flatpak")
    });

    // ── Universal — Snap ──────────────────────────────────────────────────────
    reg.register_backend({
        "snap", "/usr/bin/snap", "",
        "universal", "snap", 35,
        BACKEND_FACTORY("snap")
    });

    // ── Universal — AppImage ──────────────────────────────────────────────────
    reg.register_backend({
        "appimage", "/usr/bin/appimaged", "/usr/local/bin/appimaged",
        "universal", "appimage", 40,
        BACKEND_FACTORY("appimage")
    });

    // ── Universal — npm ───────────────────────────────────────────────────────
    reg.register_backend({
        "npm", "/usr/bin/npm", "/usr/local/bin/npm",
        "universal", "npm", 45,
        BACKEND_FACTORY("npm")
    });

    // ── Universal — pip ───────────────────────────────────────────────────────
    reg.register_backend({
        "pip", "/usr/bin/pip3", "/usr/local/bin/pip3",
        "universal", "pip", 45,
        BACKEND_FACTORY("pip")
    });

    // ── Universal — cargo ─────────────────────────────────────────────────────
    reg.register_backend({
        "cargo", "/usr/bin/cargo", "/usr/local/bin/cargo",
        "universal", "rust", 45,
        BACKEND_FACTORY("cargo")
    });

    // ── Universal — gem ───────────────────────────────────────────────────────
    reg.register_backend({
        "gem", "/usr/bin/gem", "/usr/local/bin/gem",
        "universal", "ruby", 45,
        BACKEND_FACTORY("gem")
    });

    // ── Universal — wine ──────────────────────────────────────────────────────
    reg.register_backend({
        "wine", "/usr/bin/wine", "/usr/local/bin/wine",
        "universal", "windows-compat", 50,
        BACKEND_FACTORY("wine")
    });

    // ── Universal — amber ─────────────────────────────────────────────────────
    reg.register_backend({
        "amber", "/usr/bin/amber", "/usr/local/bin/amber",
        "universal", "amber", 10,
        BACKEND_FACTORY("amber")
    });

    // ── Universal — spk ───────────────────────────────────────────────────────
    reg.register_backend({
        "spk", "/usr/bin/sspm", "",  // spk is built-in to sspm
        "universal", "sspm", 5,
        BACKEND_FACTORY("spk")
    });

    // ── Linux — .run files ────────────────────────────────────────────────────
    reg.register_backend({
        "run", "", "",  // no binary needed, it's a script runner
        "linux", "installer", 55,
        BACKEND_FACTORY("run")
    });

    return reg;
}

} // namespace sspm
