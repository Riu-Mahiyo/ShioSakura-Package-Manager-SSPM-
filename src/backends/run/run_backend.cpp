#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace sspm::backends {

/**
 * RunBackend: Handles .run self-extracting installers on Linux.
 * These are typically shell scripts or binaries that perform their own installation.
 */
class RunBackend : public Backend {
public:
    std::string name() const override { return "run"; }

    bool is_available() const override {
#ifdef SSPM_LINUX
        return true;
#else
        return false;
#endif
    }

    Result install(const Package& pkg) override {
        // In the context of .run files, the package name is often the path to the installer
        std::string path = pkg.name;
        
        if (!fs::exists(path)) {
            return { false, "", "Run file not found: " + path };
        }

        if (fs::path(path).extension() != ".run") {
            return { false, "", "Not a .run file: " + path };
        }

        std::cout << "[run] Preparing to execute installer: " << path << "\n";

        // Ensure it's executable
        std::error_code ec;
        auto perms = fs::status(path, ec).permissions();
        if (ec) return { false, "", "Failed to check permissions: " + ec.message() };
        
        fs::permissions(path, perms | fs::perms::owner_exec | fs::perms::group_exec, fs::perm_options::add, ec);
        if (ec) return { false, "", "Failed to set executable bit: " + ec.message() };

        // Execute the .run file
        // Note: .run files often need root privileges, but ExecEngine handles environment scrubbing
        int ret = ExecEngine::run(path, { "--mode", "unattended" }); // Many .run installers support unattended mode
        
        if (ret != 0) {
            // Try without arguments if unattended failed or isn't supported
            ret = ExecEngine::run(path, {});
        }

        return { ret == 0, ret == 0 ? "Installed via " + path : "", ret != 0 ? ".run execution failed (exit code " + std::to_string(ret) + ")" : "" };
    }

    Result remove(const Package& pkg) override {
        // .run files don't have a standard uninstall method.
        // Some might support --uninstall, but it's not guaranteed.
        return { false, "", "Direct removal of .run packages is not supported. Please use the application's own uninstaller if available." };
    }

    SearchResult search(const std::string& query) override {
        // Cannot search for .run files unless we index a specific directory.
        return { {}, true, "" };
    }

    Result update() override { return { true, "No-op for .run backend", "" }; }
    Result upgrade() override { return { false, "", "Upgrade not supported for .run packages" }; }
    
    PackageList list_installed() override {
        // Tracking installed .run packages would require SkyDB integration in the install step.
        return {};
    }

    std::optional<Package> info(const std::string& name) override {
        return std::nullopt;
    }
};

// Auto-register with the registry
struct RunRegister {
    RunRegister() {
        BackendRegistry::register_factory("run", []() {
            return std::make_shared<RunBackend>();
        });
    }
} _run_register;

} // namespace sspm::backends
