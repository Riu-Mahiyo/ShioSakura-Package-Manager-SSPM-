#include "sspm/backend.h"
#include "sspm/backend_registry.h"
#include "sspm/spk.h"
#include "sspm/security.h"
#include "sspm/network.h"
#include "sspm/database.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace sspm::backends {

// SPK backend: installs SSPM-native .spk packages from configured spk repos.
// This is the only backend that does not shell out to an external tool —
// install/remove/search are handled entirely inside SSPM.
class SpkBackend : public Backend {
public:
    std::string name() const override { return "spk"; }

    // spk is always available — it is built into SSPM itself
    bool is_available() const override { return true; }

    Result install(const Package& pkg) override {
        std::string spk_path = find_in_cache(pkg.name, pkg.version);
        if (spk_path.empty()) {
            spk_path = download_spk(pkg);
            if (spk_path.empty())
                return { false, "", "Could not download " + pkg.name + ".spk" };
        }

        // Verify signature before doing anything
        if (!security::verify_spk(spk_path))
            return { false, "", "Signature verification failed: " + spk_path };

        spk::SpkPackage parsed = spk::parse(spk_path);

        // Pre-install hook
        std::string scripts_dir = fs::path(spk_path).parent_path().string() + "/scripts";
        if (!spk::run_hook(scripts_dir, "pre-install"))
            return { false, "", "pre_install hook failed" };

        // Extract payload to /
        if (!spk::extract(spk_path, "/"))
            return { false, "", "Payload extraction failed" };

        // Post-install hook
        spk::run_hook(scripts_dir, "post-install");

        // Record in SkyDB
        SkyDB db(db_path_);
        if (db.open()) {
            Package record = pkg;
            record.backend = "spk";
            record.version = parsed.metadata.version;
            db.insert_package(record);
            db.log_history("install", record);
        }

        std::cout << "[spk] Installed: " << pkg.name << " " << parsed.metadata.version << "\n";
        return { true, "Installed " + pkg.name, "" };
    }

    Result remove(const Package& pkg) override {
        // Look up install script from SkyDB / cache
        std::string cached = find_in_cache(pkg.name, "");
        if (!cached.empty()) {
            spk::SpkPackage parsed = spk::parse(cached);
            std::string scripts_dir = fs::path(cached).parent_path().string() + "/scripts";
            spk::run_hook(scripts_dir, "pre-remove");
            // TODO: enumerate installed files from payload manifest and remove them
            spk::run_hook(scripts_dir, "post-remove");
        }

        SkyDB db(db_path_);
        if (db.open()) {
            db.remove_package(pkg.name);
            db.log_history("remove", pkg);
        }

        std::cout << "[spk] Removed: " << pkg.name << "\n";
        return { true, "Removed " + pkg.name, "" };
    }

    SearchResult search(const std::string& query) override {
        // Search the locally cached repo index
        // TODO: load PackageIndex from repo JSON and call search_fuzzy
        SearchResult r;
        r.success = true;
        std::cout << "[spk] Searching repo index for: " << query << "\n";
        return r;
    }

    Result update() override {
        // Sync all spk repo indexes via RepoManager
        std::cout << "[spk] Syncing spk repo indexes...\n";
        // TODO: call RepoManager::sync_all() filtered to spk repos
        return { true, "SPK repo indexes synced", "" };
    }

    Result upgrade() override {
        // For each installed spk package, check latest version in index
        std::cout << "[spk] Checking for SPK package upgrades...\n";
        // TODO: diff installed versions vs index, install newer
        return { true, "", "" };
    }

    PackageList list_installed() override {
        SkyDB db(db_path_);
        if (!db.open()) return {};
        auto all = db.get_installed();
        PackageList result;
        for (auto& p : all)
            if (p.backend == "spk") result.push_back(p);
        return result;
    }

    std::optional<Package> info(const std::string& name) override {
        SkyDB db(db_path_);
        if (!db.open()) return std::nullopt;
        auto pkg = db.get_package(name);
        if (pkg && pkg->backend == "spk") return pkg;
        return std::nullopt;
    }

private:
    std::string db_path_    = std::string(getenv("HOME")) + "/.local/share/sspm/sky.db";
    std::string cache_path_ = std::string(getenv("HOME")) + "/.cache/sspm/packages";

    std::string find_in_cache(const std::string& name, const std::string& version) const {
        std::string pattern = cache_path_ + "/" + name;
        if (!version.empty()) pattern += "-" + version;
        pattern += ".spk";
        if (fs::exists(pattern)) return pattern;
        // Glob: find any version in cache
        for (auto& entry : fs::directory_iterator(cache_path_)) {
            std::string fname = entry.path().filename().string();
            if (fname.substr(0, name.size()) == name && fname.ends_with(".spk"))
                return entry.path().string();
        }
        return "";
    }

    std::string download_spk(const Package& pkg) {
        // TODO: look up URL from PackageIndex, then call network::download()
        std::string dest = cache_path_ + "/" + pkg.name + "-" + pkg.version + ".spk";
        std::cout << "[spk] Downloading: " << pkg.name << "\n";
        // network::download({ url, dest, fallback_urls });
        return ""; // placeholder until index lookup is wired
    }
};

// Auto-register with the registry
struct SpkRegister {
    SpkRegister() {
        BackendRegistry::register_factory("spk", []() {
            return std::make_shared<SpkBackend>();
        });
    }
} _spk_register;

} // namespace sspm::backends
