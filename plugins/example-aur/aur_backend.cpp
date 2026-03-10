// aur_backend.cpp — AUR backend plugin for SSPM
// Build: g++ -std=c++20 -shared -fPIC -o libaur.so aur_backend.cpp
//
// This is a skeleton showing how to write a backend plugin.
// A real AUR backend would call aurweb RPC: https://aur.archlinux.org/rpc/v5/

#include "sspm/backend.h"
#include <iostream>
#include <vector>

class AurBackend : public sspm::Backend {
public:
    std::string name() const override { return "aur"; }

    // AUR is only available on Arch/Arch-based distros with yay or paru installed
    bool is_available() const override {
        return std::filesystem::exists("/usr/bin/yay") ||
               std::filesystem::exists("/usr/bin/paru");
    }

    sspm::Result install(const sspm::Package& pkg) override {
        std::string helper = std::filesystem::exists("/usr/bin/paru") ? "paru" : "yay";
        std::string cmd = helper + " -S --noconfirm " + pkg.name;
        int rc = std::system(cmd.c_str());
        return { rc == 0, rc == 0 ? "Installed " + pkg.name : "", "AUR install failed" };
    }

    sspm::Result remove(const sspm::Package& pkg) override {
        int rc = std::system(("pacman -Rs --noconfirm " + pkg.name).c_str());
        return { rc == 0, "", "Remove failed" };
    }

    sspm::SearchResult search(const std::string& query) override {
        sspm::SearchResult r;
        r.success = true;
        // Real impl: GET https://aur.archlinux.org/rpc/v5/search/<query>
        std::cout << "[aur] Searching AUR for: " << query << "\n";
        std::cout << "[aur] (real impl would query https://aur.archlinux.org/rpc/v5/search/" << query << ")\n";
        return r;
    }

    sspm::Result update() override {
        std::string helper = std::filesystem::exists("/usr/bin/paru") ? "paru" : "yay";
        int rc = std::system((helper + " -Sy").c_str());
        return { rc == 0, "", "" };
    }

    sspm::Result upgrade() override {
        std::string helper = std::filesystem::exists("/usr/bin/paru") ? "paru" : "yay";
        int rc = std::system((helper + " -Syu --noconfirm").c_str());
        return { rc == 0, "", "" };
    }

    sspm::PackageList list_installed() override { return {}; }
    std::optional<sspm::Package> info(const std::string&) override { return std::nullopt; }
};

// ── Plugin ABI ────────────────────────────────────────────────────────────────
// These two functions are the dlsym targets in PluginManager::load_so()

extern "C" sspm::Backend* create_backend() {
    return new AurBackend();
}

extern "C" void destroy_backend(sspm::Backend* b) {
    delete b;
}
