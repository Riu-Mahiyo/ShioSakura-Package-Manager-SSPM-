#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>

namespace sspm::backends {

class PacmanBackend : public Backend {
public:
    std::string name() const override { return "pacman"; }

    bool is_available() const override {
        return ExecEngine::exists("pacman");
    }

    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int ret = ExecEngine::run("pacman", { "-S", "--noconfirm", pkg.name });
        return { ret == 0, ret == 0 ? "Installed " + pkg.name : "", ret != 0 ? "pacman failed" : "" };
    }

    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int ret = ExecEngine::run("pacman", { "-Rs", "--noconfirm", pkg.name });
        return { ret == 0, "", ret != 0 ? "pacman -Rs failed" : "" };
    }

    SearchResult search(const std::string& query) override {
        auto val = InputValidator::safe_string(query, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run("pacman", { "-Ss", query });
        return { {}, true, "" };
    }

    Result update() override {
        int ret = ExecEngine::run("pacman", { "-Sy" });
        return { ret == 0, "", ret != 0 ? "pacman -Sy failed" : "" };
    }

    Result upgrade() override {
        int ret = ExecEngine::run("pacman", { "-Syu", "--noconfirm" });
        return { ret == 0, "", ret != 0 ? "pacman -Syu failed" : "" };
    }

    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& name) override { return std::nullopt; }
};

// Auto-register with the registry
struct PacmanRegister {
    PacmanRegister() {
        BackendRegistry::register_factory("pacman", []() {
            return std::make_shared<PacmanBackend>();
        });
    }
} _pacman_register;

} // namespace sspm::backends
