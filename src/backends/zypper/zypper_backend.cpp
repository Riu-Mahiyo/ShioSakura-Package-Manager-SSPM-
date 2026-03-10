#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>

namespace sspm::backends {

class ZypperBackend : public Backend {
public:
    std::string name() const override { return "zypper"; }
    bool is_available() const override {
        return ExecEngine::exists("zypper");
    }
    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("zypper", { "install", "-y", pkg.name });
        return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"zypper install failed":"" };
    }
    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run("zypper", { "remove", "-y", pkg.name });
        return { r==0, "", r!=0?"zypper remove failed":"" };
    }
    SearchResult search(const std::string& q) override {
        auto val = InputValidator::safe_string(q, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run("zypper", { "search", q }); 
        return { {}, true, "" };
    }
    Result update() override {
        int r = ExecEngine::run("zypper", { "ref" }); 
        return { r==0, "", "" };
    }
    Result upgrade() override {
        int r = ExecEngine::run("zypper", { "up", "-y" }); 
        return { r==0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }
};

// Auto-register with the registry
struct ZypperRegister {
    ZypperRegister() {
        BackendRegistry::register_factory("zypper", []() {
            return std::make_shared<ZypperBackend>();
        });
    }
} _zypper_register;

} // namespace sspm::backends
