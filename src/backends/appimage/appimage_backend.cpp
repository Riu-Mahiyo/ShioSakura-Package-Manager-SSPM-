#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/exec.h"
#include "sspm/validator.h"
#include "sspm/network.h"
#include "sspm/log.h"
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace sspm::backends {

/**
 * AppImageBackend: Handles .AppImage files.
 * Supports both system-wide 'appimage' tool and manual management (portable mode).
 */
class AppImageBackend : public Backend {
public:
    std::string name() const override { return "appimage"; }

    bool is_available() const override {
        // AppImage needs FUSE on Linux
#ifdef SSPM_LINUX
        return fs::exists("/dev/fuse") || ExecEngine::exists("appimage");
#else
        return false;
#endif
    }

    Result install(const Package& pkg) override {
        // If it looks like a URL, download and install manually
        if (pkg.name.rfind("http://", 0) == 0 || pkg.name.rfind("https://", 0) == 0) {
            return install_portable(pkg.name, "");
        }

        // If 'appimage' tool exists, use it
        if (ExecEngine::exists("appimage")) {
            auto val = InputValidator::pkg(pkg.name);
            if (!val) return { false, "", "Invalid package name: " + val.reason };

            int r = ExecEngine::run("appimage", { "install", pkg.name });
            return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"appimage install failed":"" };
        }

        return { false, "", "AppImage tool not found and input is not a URL" };
    }

    Result remove(const Package& pkg) override {
        // Try portable removal first
        std::string dest_file = portable_dir() + "/" + pkg.name + "/" + pkg.name + ".AppImage";
        if (fs::exists(dest_file)) {
            std::error_code ec;
            fs::remove_all(portable_dir() + "/" + pkg.name, ec);
            fs::remove(bin_dir() + "/" + pkg.name, ec);
            return { true, "Removed portable AppImage: " + pkg.name, "" };
        }

        // Otherwise use system tool
        if (ExecEngine::exists("appimage")) {
            auto val = InputValidator::pkg(pkg.name);
            if (!val) return { false, "", "Invalid package name: " + val.reason };

            int r = ExecEngine::run("appimage", { "remove", pkg.name });
            return { r==0, "", r!=0?"appimage remove failed":"" };
        }

        return { false, "", "AppImage package not found" };
    }

    SearchResult search(const std::string& q) override {
        if (ExecEngine::exists("appimage")) {
            auto val = InputValidator::safe_string(q, 128);
            if (!val) return { {}, false, "Invalid search query: " + val.reason };
            ExecEngine::run("appimage", { "search", q }); 
        }
        return { {}, true, "" };
    }

    Result update() override {
        if (ExecEngine::exists("appimage")) {
            int r = ExecEngine::run("appimage", { "update" }); 
            return { r==0, "", "" };
        }
        return { true, "No-op for portable AppImages", "" };
    }

    Result upgrade() override {
        if (ExecEngine::exists("appimage")) {
            int r = ExecEngine::run("appimage", { "upgrade" }); 
            return { r==0, "", "" };
        }
        return { false, "", "Upgrade not supported for portable AppImages" };
    }

    PackageList list_installed() override {
        PackageList list;
        if (fs::exists(portable_dir())) {
            for (auto& entry : fs::directory_iterator(portable_dir())) {
                if (entry.is_directory()) {
                    Package p;
                    p.name = entry.path().filename().string();
                    p.backend = "appimage";
                    p.description = "Portable AppImage";
                    list.push_back(p);
                }
            }
        }
        return list;
    }

    std::optional<Package> info(const std::string& n) override { return std::nullopt; }

private:
    static std::string portable_dir() {
        const char* home = getenv("HOME");
        return std::string(home ? home : "/tmp") + "/.local/share/sspm/portable";
    }

    static std::string bin_dir() {
        const char* home = getenv("HOME");
        return std::string(home ? home : "/tmp") + "/.local/bin";
    }

    Result install_portable(const std::string& url, std::string app_name) {
        if (app_name.empty()) {
            // Extract name from URL
            size_t last_slash = url.find_last_of('/');
            if (last_slash != std::string::npos) {
                app_name = url.substr(last_slash + 1);
                // Remove .AppImage extension
                if (app_name.size() > 9 && app_name.substr(app_name.size() - 9) == ".AppImage")
                    app_name = app_name.substr(0, app_name.size() - 9);
            }
        }
        if (app_name.empty()) return { false, "", "Could not determine app name from URL" };

        std::string dest_dir = portable_dir() + "/" + app_name;
        std::string dest_file = dest_dir + "/" + app_name + ".AppImage";

        std::error_code ec;
        fs::create_directories(dest_dir, ec);
        if (ec) return { false, "", "Failed to create directory: " + dest_dir };

        std::cout << "[appimage] Downloading: " << url << "\n";
        network::DownloadRequest req;
        req.url = url;
        req.dest_path = dest_file;
        req.timeout_sec = 300;
        
        auto res = network::download(req);
        if (!res.success) return { false, "", "Download failed: " + res.error };

        // chmod +x
        ::chmod(dest_file.c_str(), 0755);

        // Symlink
        fs::create_directories(bin_dir(), ec);
        std::string link_path = bin_dir() + "/" + app_name;
        if (fs::exists(link_path)) fs::remove(link_path);
        fs::create_symlink(dest_file, link_path, ec);

        return { true, "Installed portable AppImage to " + dest_file, "" };
    }
};

// Auto-register with the registry
struct AppImageRegister {
    AppImageRegister() {
        BackendRegistry::register_factory("appimage", []() {
            return std::make_shared<AppImageBackend>();
        });
    }
} _appimage_register;

} // namespace sspm::backends
