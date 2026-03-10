#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>

namespace sspm::backends {

class DnfBackend : public Backend {
public:
    std::string name() const override { return "dnf"; }
    bool is_available() const override {
        return ExecEngine::exists("dnf");
    }
    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("dnf", { "install", "-y", pkg.name });
        return { r == 0, r == 0 ? "Installed "+pkg.name : "", r != 0 ? "dnf failed" : "" };
    }
    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("dnf", { "remove", "-y", pkg.name });
        return { r == 0, "", r != 0 ? "dnf remove failed" : "" };
    }
    SearchResult search(const std::string& q) override {
        auto val = InputValidator::safe_string(q, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run("dnf", { "search", q }); 
        return { {}, true, "" };
    }
    Result update() override {
        int r = ExecEngine::run("dnf", { "check-update" }); 
        return { r == 0 || r == 100, "", "" }; // dnf check-update returns 100 if updates available
    }
    Result upgrade() override {
        int r = ExecEngine::run("dnf", { "upgrade", "-y" }); 
        return { r == 0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& name) override { return std::nullopt; }
};

// Auto-register with the registry
struct DnfRegister {
    DnfRegister() {
        BackendRegistry::register_factory("dnf", []() {
            return std::make_shared<DnfBackend>();
        });
    }
} _dnf_register;

} // namespace sspm::backends
