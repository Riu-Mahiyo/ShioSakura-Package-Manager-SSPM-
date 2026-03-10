#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace sspm::backends {

class BrewBackend : public Backend {
public:
    std::string name() const override { return "brew"; }

    bool is_available() const override {
        // 1. Check if brew is in PATH
        if (ExecEngine::exists("brew")) return true;

        // 2. Check standard Linuxbrew paths if not in PATH
        if (fs::exists("/home/linuxbrew/.linuxbrew/bin/brew")) return true;

        return false;
    }

    Result install(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        if (!is_available()) {
            auto res = auto_install();
            if (!res.success) return res;
        }
        
        int r = ExecEngine::run(get_brew_path(), { "install", pkg.name });
        return { r == 0, r == 0 ? "Installed " + pkg.name : "", r != 0 ? "brew install failed" : "" };
    }

    Result remove(const Package& pkg) override {
        auto val = InputValidator::pkg(pkg.name);
        if (!val) return { false, "", "Invalid package name: " + val.reason };

        int r = ExecEngine::run(get_brew_path(), { "remove", pkg.name });
        return { r == 0, "", r != 0 ? "brew remove failed" : "" };
    }

    SearchResult search(const std::string& q) override {
        auto val = InputValidator::safe_string(q, 128);
        if (!val) return { {}, false, "Invalid search query: " + val.reason };

        ExecEngine::run(get_brew_path(), { "search", q }); 
        return { {}, true, "" };
    }

    Result update() override {
        int r = ExecEngine::run(get_brew_path(), { "update" }); 
        return { r == 0, "", "" };
    }

    Result upgrade() override {
        int r = ExecEngine::run(get_brew_path(), { "upgrade" }); 
        return { r == 0, "", "" };
    }

    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }

private:
    std::string get_brew_path() const {
        std::string p = ExecEngine::find_in_path("brew");
        if (!p.empty()) return p;

        if (fs::exists("/home/linuxbrew/.linuxbrew/bin/brew")) 
            return "/home/linuxbrew/.linuxbrew/bin/brew";
        
        const char* home = std::getenv("HOME");
        if (home) {
            std::string user_brew = std::string(home) + "/.linuxbrew/bin/brew";
            if (fs::exists(user_brew)) return user_brew;
        }
        
        return "brew"; // fallback
    }

    Result auto_install() {
        std::cout << "[brew] Homebrew not found. Attempting automatic installation...\n";
        
        // Securely install homebrew without using shell -c if possible, 
        // but the official installer is a script meant for shell.
        // We'll use a slightly safer approach by fetching it ourselves or 
        // at least using ExecEngine to run bash with the script.
        
        // For now, let's keep it simple but use ExecEngine for the bash call.
        // The script itself is from a trusted source (Homebrew).
        
        // We need curl to get the script.
        std::string script_path = "/tmp/brew_install.sh";
        ExecEngine::run("curl", { "-fsSL", "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh", "-o", script_path });
        
        int r = ExecEngine::run("bash", { script_path });
        fs::remove(script_path);

        if (r != 0) {
            return { false, "", "Homebrew automatic installation failed" };
        }

        // Post-install configuration
        const char* home = std::getenv("HOME");
        if (home) {
            std::string shell_rc;
            if (fs::exists(std::string(home) + "/.zshrc")) shell_rc = std::string(home) + "/.zshrc";
            else if (fs::exists(std::string(home) + "/.bashrc")) shell_rc = std::string(home) + "/.bashrc";

            if (!shell_rc.empty()) {
                std::cout << "[brew] Configuring shell environment in " << shell_rc << "\n";
                // Still need a bit of shell for redirection, but we can do it more safely.
                std::string cmd = "echo 'eval \"$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)\"' >> " + shell_rc;
                std::system(cmd.c_str()); 
            }
        }

        return { true, "Homebrew installed and configured successfully", "" };
    }
};

// Auto-register with the registry
struct BrewRegister {
    BrewRegister() {
        BackendRegistry::register_factory("brew", []() {
            return std::make_shared<BrewBackend>();
        });
    }
} _brew_register;

} // namespace sspm::backends
