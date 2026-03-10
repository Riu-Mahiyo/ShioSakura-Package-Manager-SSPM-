#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>

namespace sspm::backends {

class SnapBackend : public Backend {
public:
    std::string name() const override { return "snap"; }
    bool is_available() const override {
        return ExecEngine::exists("snap");
    }
    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("snap", { "install", pkg.name });
        return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"snap install failed":"" };
    }
    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("snap", { "remove", pkg.name });
        return { r==0, "", r!=0?"snap remove failed":"" };
    }
    SearchResult search(const std::string& q) override {
        auto val = InputValidator::safe_string(q, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run("snap", { "search", q }); 
        return { {}, true, "" };
    }
    Result update() override {
        int r = ExecEngine::run("snap", { "refresh" }); 
        return { r==0, "", "" };
    }
    Result upgrade() override {
        int r = ExecEngine::run("snap", { "refresh" }); 
        return { r==0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }
};

// Auto-register with the registry
struct SnapRegister {
    SnapRegister() {
        BackendRegistry::register_factory("snap", []() {
            return std::make_shared<SnapBackend>();
        });
    }
} _snap_register;

} // namespace sspm::backends
