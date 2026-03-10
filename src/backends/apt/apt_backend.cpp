#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>
#include <sstream>

namespace sspm::backends {

class AptBackend : public Backend {
public:
    std::string name() const override { return "apt"; }

    bool is_available() const override {
        return ExecEngine::exists("apt-get");
    }

    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int ret = ExecEngine::run("apt-get", { "install", "-y", pkg.name });
        return { ret == 0, ret == 0 ? "Installed " + pkg.name : "", ret != 0 ? "apt-get failed" : "" };
    }

    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int ret = ExecEngine::run("apt-get", { "remove", "-y", pkg.name });
        return { ret == 0, "", ret != 0 ? "apt-get remove failed" : "" };
    }

    SearchResult search(const std::string& query) override {
        auto val = InputValidator::safe_string(query, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run("apt-cache", { "search", query });
        return { {}, true, "" };
    }

    Result update() override {
        int ret = ExecEngine::run("apt-get", { "update" });
        return { ret == 0, "", ret != 0 ? "apt-get update failed" : "" };
    }

    Result upgrade() override {
        int ret = ExecEngine::run("apt-get", { "upgrade", "-y" });
        return { ret == 0, "", ret != 0 ? "apt-get upgrade failed" : "" };
    }

    PackageList list_installed() override {
        // TODO: parse dpkg --list output
        return {};
    }

    std::optional<Package> info(const std::string& name) override {
        // TODO: parse apt-cache show output
        return std::nullopt;
    }
};

// Auto-register with the registry
struct AptRegister {
    AptRegister() {
        BackendRegistry::register_factory("apt", []() {
            return std::make_shared<AptBackend>();
        });
    }
} _apt_register;

} // namespace sspm::backends
