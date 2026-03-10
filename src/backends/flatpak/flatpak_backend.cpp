#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>

namespace sspm::backends {

class FlatpakBackend : public Backend {
public:
    std::string name() const override { return "flatpak"; }
    bool is_available() const override {
        return ExecEngine::exists("flatpak");
    }
    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("flatpak", { "install", "-y", "flathub", pkg.name });
        return { r == 0, r == 0 ? "Installed "+pkg.name : "", r != 0 ? "flatpak failed" : "" };
    }
    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("flatpak", { "uninstall", "-y", pkg.name });
        return { r == 0, "", r != 0 ? "flatpak remove failed" : "" };
    }
    SearchResult search(const std::string& q) override {
        auto val = InputValidator::safe_string(q, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run("flatpak", { "search", q }); 
        return { {}, true, "" };
    }
    Result update() override {
        ExecEngine::run("flatpak", { "update", "--appstream" }); 
        return { true, "", "" };
    }
    Result upgrade() override {
        int r = ExecEngine::run("flatpak", { "update", "-y" }); 
        return { r == 0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& name) override { return std::nullopt; }
};

// Auto-register with the registry
struct FlatpakRegister {
    FlatpakRegister() {
        BackendRegistry::register_factory("flatpak", []() {
            return std::make_shared<FlatpakBackend>();
        });
    }
} _flatpak_register;

} // namespace sspm::backends
