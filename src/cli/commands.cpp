#include "sspm/cli.h"
#include "sspm/database.h"
#include "sspm/cache.h"
#include "sspm/repo.h"
#include "sspm/profile.h"
#include "sspm/mirror.h"
#include "sspm/log.h"
#include "sspm/api.h"
#include "sspm/plugin.h"
#include "sspm/spk.h"
#include "sspm/security.h"
#include "sspm/backend_registry.h"
#include "../backends/amber/amber_backend.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;

static const std::string LOCK_PATH  = "/tmp/sspm.lock";

namespace sspm::cli {

// ── sspm backends ────────────────────────────────────────────────────────
void cmd_backends(const std::vector<std::string>& args) {
    auto registry = BackendRegistry::create();
    registry.probe_all_async(); // Performance: Parallel probing
    registry.print_detection_report();
}

// ── sspm lock ────────────────────────────────────────────────────────────
void cmd_lock(const std::vector<std::string>& args) {
    int fd = open(LOCK_PATH.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        std::cerr << "Error: Could not open lock file " << LOCK_PATH << "\n";
        return;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        std::cout << "Status: System is NOT locked (idle).\n";
        flock(fd, LOCK_UN);
    } else {
        std::cout << "Status: System is LOCKED (another sspm instance is running).\n";
    }
    close(fd);
}

// ── sspm amber-token <token> ─────────────────────────────────────────────
void cmd_amber_token(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::string tok = backends::AmberBackend::token();
        if (tok.empty()) std::cout << "Amber token is not set.\n";
        else std::cout << "Amber token: " << tok.substr(0, 4) << "********\n";
        return;
    }
    if (backends::AmberBackend::setToken(args[0])) {
        std::cout << "Amber token updated successfully.\n";
    } else {
        std::cerr << "Error: Failed to save Amber token.\n";
    }
}

// ── sspm fix-path [nix|brew] ─────────────────────────────────────────────
void cmd_fix_path(const std::vector<std::string>& args) {
    std::string target = args.empty() ? "all" : args[0];
    std::cout << "Fixing PATH for: " << target << "...\n";
    
    // Simple implementation: check if common paths are in PATH, if not, suggest lines for .bashrc/.zshrc
    auto path = getenv("PATH");
    std::string path_str = path ? path : "";

    auto check_and_fix = [&](const std::string& name, const std::string& bin_path, const std::string& shell_line) {
        if (path_str.find(bin_path) == std::string::npos) {
            std::cout << "[!] " << name << " path (" << bin_path << ") is missing from PATH.\n";
            std::cout << "    Suggested fix: Add this to your ~/.zshrc or ~/.bashrc:\n";
            std::cout << "    " << shell_line << "\n";
        } else {
            std::cout << "[✓] " << name << " path is correctly configured.\n";
        }
    };

    if (target == "all" || target == "brew") {
        check_and_fix("Homebrew", "/opt/homebrew/bin", "export PATH=\"/opt/homebrew/bin:$PATH\"");
    }
    if (target == "all" || target == "nix") {
        check_and_fix("Nix", "/nix/var/nix/profiles/default/bin", "export PATH=\"$HOME/.nix-profile/bin:/nix/var/nix/profiles/default/bin:$PATH\"");
    }
}

// ── sspm version ──────────────────────────────────────────────────────────
void cmd_version(const std::vector<std::string>& args) {
    std::cout << "🌸 SSPM — ShioSakura Package Manager\n"
              << "   Version: v4.0.0-Sakura\n"
              << "   Platform: " << 
#ifdef SSPM_LINUX
                 "Linux"
#elif defined(SSPM_MACOS)
                 "macOS"
#elif defined(SSPM_WINDOWS)
                 "Windows"
#else
                 "Universal"
#endif
              << "\n"
              << "   Build: " << __DATE__ << " " << __TIME__ << "\n"
              << "   License: GPLv2\n";
}

// ── sspm install <pkg> [--backend <n>] [--dry-run] ────────────────────────
void cmd_install(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: sspm install <package> [options]\n\n"
                  << "Options:\n"
                  << "  --backend <name>   Force a specific backend (apt, brew, nix, etc.)\n"
                  << "  --dry-run          Show what would be done without making changes\n"
                  << "  --no-verify        Skip signature verification (Insecure!)\n"
                  << "  --user             Install for current user only (if supported)\n";
        return;
    }
    
    std::string pkg_name = args[0];
    bool dry_run = false;
    bool no_verify = false;
    std::string forced_backend;

    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--dry-run") dry_run = true;
        if (args[i] == "--no-verify") no_verify = true;
        if (args[i] == "--backend" && i+1 < args.size()) forced_backend = args[++i];
    }

    if (dry_run) {
        std::cout << "🌸 [dry-run] Analysis for: " << pkg_name << "\n";
        // TODO: BackendRegistry::best_for(pkg_name) -> dependency graph
        return;
    }

    // Security: Drop privileges before running any scripts or network ops
    // sspm will re-acquire root (via sudo/polkit) only when calling backend->install()
    security::drop_privileges();

    std::cout << "🌸 Installing: " << pkg_name << "...\n";
    SSPM_INFO("install start: " + pkg_name);

    // TODO: 
    // 1. Resolve backend (parallel probe)
    // 2. Build transaction
    // 3. Verify signature (security::verify_spkg)
    // 4. Execute (with audit log)
}

// ── sspm remove <pkg> ────────────────────────────────────────────────────
void cmd_remove(const std::vector<std::string>& args) {
    if (args.empty()) { std::cerr << "Usage: sspm remove <package>\n"; return; }
    std::cout << "Removing: " << args[0] << "\n";
    SSPM_INFO("remove: " + args[0]);
}

// ── sspm search <query> [--json] [--regex <pat>] ────────────────────────
void cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) { std::cerr << "Usage: sspm search <query>\n"; return; }
    bool json = false;
    bool use_regex = false;
    std::string query = args[0];
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--json") json = true;
        if (args[i] == "--regex") use_regex = true;
    }
    std::cout << "Searching for: " << query;
    if (use_regex) std::cout << " (regex)";
    std::cout << "\n";
    // TODO: PackageIndex::search_fuzzy / search_regex
}

// ── sspm update ──────────────────────────────────────────────────────────
void cmd_update(const std::vector<std::string>& args) {
    std::cout << "Syncing package databases...\n";
    RepoManager repo(DB_PATH, CACHE_PATH);
    repo.sync_all();
    SSPM_INFO("update: repo sync done");
}

// ── sspm upgrade ─────────────────────────────────────────────────────────
void cmd_upgrade(const std::vector<std::string>& args) {
    std::cout << "Upgrading all packages...\n";
    // TODO: iterate all installed packages, call backend->upgrade()
    SSPM_INFO("upgrade: all packages");
}

// ── sspm list [--format table|json] ─────────────────────────────────────
void cmd_list(const std::vector<std::string>& args) {
    bool json = false;
    for (auto& a : args) if (a == "--json") json = true;

    SkyDB db(DB_PATH);
    if (!db.open()) { std::cerr << "Cannot open database\n"; return; }
    auto pkgs = db.get_installed();

    if (json) {
        std::cout << "[\n";
        for (size_t i = 0; i < pkgs.size(); i++) {
            std::cout << "  {\"name\":\"" << pkgs[i].name
                      << "\",\"version\":\"" << pkgs[i].version
                      << "\",\"backend\":\"" << pkgs[i].backend << "\"}";
            if (i+1 < pkgs.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";
    } else {
        std::cout << std::left << std::setw(30) << "Name"
                  << std::setw(15) << "Version"
                  << std::setw(15) << "Backend" << "\n";
        std::cout << std::string(60, '-') << "\n";
        for (auto& p : pkgs) {
            std::cout << std::setw(30) << p.name
                      << std::setw(15) << p.version
                      << std::setw(15) << p.backend << "\n";
        }
        std::cout << pkgs.size() << " packages installed\n";
    }
}

// ── sspm info <pkg> ──────────────────────────────────────────────────────
void cmd_info(const std::vector<std::string>& args) {
    if (args.empty()) { std::cerr << "Usage: sspm info <package>\n"; return; }
    SkyDB db(DB_PATH);
    if (!db.open()) return;
    auto pkg = db.get_package(args[0]);
    if (!pkg) { std::cout << "Package not found: " << args[0] << "\n"; return; }
    std::cout << "Name:        " << pkg->name        << "\n"
              << "Version:     " << pkg->version     << "\n"
              << "Description: " << pkg->description << "\n"
              << "Backend:     " << pkg->backend     << "\n"
              << "Repo:        " << pkg->repo        << "\n"
              << "Platform:    " << pkg->platform    << "\n"
              << "Hash:        " << pkg->hash        << "\n"
              << "Installed:   " << pkg->install_time << "\n"
              << "Desktop Reg: " << (pkg->desktop_registered ? "Yes" : "No") << "\n"
              << "MIME Reg:    " << (pkg->mime_registered ? "Yes" : "No") << "\n";
}

// ── sspm history ─────────────────────────────────────────────────────────
void cmd_history(const std::vector<std::string>& args) {
    SkyDB db(DB_PATH);
    if (!db.open()) return;
    auto hist = db.get_history(50);
    for (auto& h : hist) std::cout << h << "\n";
}

// ── sspm rollback ────────────────────────────────────────────────────────
void cmd_rollback(const std::vector<std::string>& args) {
    std::cout << "Rolling back last transaction...\n";
    // TODO: Transaction::rollback(last_tx_id)
}

// ── sspm verify <pkg> ────────────────────────────────────────────────────
void cmd_verify(const std::vector<std::string>& args) {
    if (args.empty()) { std::cerr << "Usage: sspm verify <package|file.spk>\n"; return; }
    if (args[0].size() > 4 && args[0].substr(args[0].size()-4) == ".spk") {
        bool ok = security::verify_spk(args[0]);
        std::cout << (ok ? "✅ Valid" : "❌ Invalid") << ": " << args[0] << "\n";
    } else {
        std::cout << "Verifying installed package: " << args[0] << "\n";
        // TODO: verify via SkyDB hash record
    }
}

// ── sspm repo <sub> ──────────────────────────────────────────────────────
void cmd_repo(const std::vector<std::string>& args) {
    RepoManager repo(DB_PATH, CACHE_PATH);
    if (args.empty() || args[0] == "list") {
        auto repos = repo.list();
        for (auto& r : repos)
            std::cout << r.name << "  " << r.url << "\n";
        return;
    }
    if (args[0] == "add" && args.size() >= 3)    { repo.add(args[1], args[2]); return; }
    if (args[0] == "remove" && args.size() >= 2) { repo.remove(args[1]); return; }
    if (args[0] == "sync")                        { repo.sync_all(); return; }
    std::cerr << "Usage: sspm repo add <name> <url> | remove <name> | sync | list\n";
}

// ── sspm cache <sub> ─────────────────────────────────────────────────────
void cmd_cache(const std::vector<std::string>& args) {
    CacheManager cache(CACHE_PATH);
    if (args.empty() || args[0] == "size") {
        uint64_t sz = cache.total_size();
        std::cout << "Cache size: " << sz / 1024 / 1024 << " MB\n";
        return;
    }
    if (args[0] == "clean") { cache.clean(); return; }
    if (args[0] == "prune") { cache.prune(30); return; }
    std::cerr << "Usage: sspm cache size | clean | prune\n";
}

// ── sspm config <sub> ────────────────────────────────────────────────────
void cmd_config(const std::vector<std::string>& args) {
    if (args.empty()) { std::cout << "Config file: " << CFG_PATH << "\n"; return; }
    if (args[0] == "set" && args.size() >= 3) {
        std::cout << "Setting " << args[1] << " = " << args[2] << "\n";
        // TODO: YAML write
    }
    if (args[0] == "get" && args.size() >= 2) {
        std::cout << "Getting " << args[1] << "\n";
        // TODO: YAML read
    }
}

// ── sspm profile <sub> ───────────────────────────────────────────────────
void cmd_profile(const std::vector<std::string>& args) {
    ProfileManager pm(DB_PATH);
    if (args.empty() || args[0] == "list") {
        for (auto& p : pm.list())
            std::cout << p.name << "  (" << p.packages.size() << " packages)\n";
        return;
    }
    if (args[0] == "create" && args.size() >= 2) { pm.create(args[1]); return; }
    if (args[0] == "apply"  && args.size() >= 2) { pm.apply(args[1]); return; }
    if (args[0] == "delete" && args.size() >= 2) { pm.remove(args[1]); return; }
    std::cerr << "Usage: sspm profile create|apply|list|delete <name>\n";
}

// ── sspm mirror <sub> ────────────────────────────────────────────────────
void cmd_mirror(const std::vector<std::string>& args) {
    MirrorManager mm(CFG_PATH);
    if (args.empty() || args[0] == "list") {
        for (auto& m : mm.list(""))
            std::cout << std::left << std::setw(20) << m.name
                      << std::setw(8)  << m.country
                      << (m.latency_ms > 0 && m.latency_ms < 9000 ? std::to_string((int)m.latency_ms)+"ms" : "?")
                      << "  " << m.url << "\n";
        return;
    }
    if (args[0] == "test") {
        std::string backend = args.size() >= 2 ? args[1] : "";
        mm.rank_by_latency(backend);
        return;
    }
    if (args[0] == "select" && args.size() >= 2)  { mm.select(args[1]); return; }
    if (args[0] == "auto") {
        std::string backend = args.size() >= 2 ? args[1] : "";
        mm.auto_select(backend);
        return;
    }
    std::cerr << "Usage: sspm mirror list | test [backend] | select <name> | auto [backend]\n";
}

// ── sspm log [tail] ──────────────────────────────────────────────────────
void cmd_log(const std::vector<std::string>& args) {
    Logger::instance().init(LOG_PATH);
    if (!args.empty() && args[0] == "tail") {
        Logger::instance().tail();
    } else {
        Logger::instance().print_recent(50);
    }
}

// ── sspm daemon start|stop|status ────────────────────────────────────────
void cmd_daemon(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "start") {
        api::Daemon daemon(7373);
        daemon.start();
    } else if (args[0] == "status") {
        std::cout << "SSPM daemon: checking port 7373...\n";
        // TODO: ping /health
    } else if (args[0] == "stop") {
        std::cout << "Stopping daemon...\n";
        // TODO: send SIGTERM to PID file
    }
}

// ── sspm plugin install|remove|list ─────────────────────────────────────
void cmd_plugin(const std::vector<std::string>& args) {
    plugin::PluginManager pm(PLUGIN_DIR);
    if (args.empty() || args[0] == "list") {
        for (auto& p : pm.list())
            std::cout << p.name << "  " << p.version << "  " << p.description << "\n";
        return;
    }
    if (args[0] == "install" && args.size() >= 2) {
        if (!pm.install(args[1]))
            std::cerr << "plugin install failed: " << args[1] << "\n";
        return;
    }
    if (args[0] == "remove"  && args.size() >= 2) { pm.remove(args[1]); return; }
    std::cerr << "Usage: sspm plugin install|remove|list <name>\n";
}

// ── sspm build <dir> ─────────────────────────────────────────────────────
void cmd_build(const std::vector<std::string>& args) {
    if (args.empty()) { std::cerr << "Usage: sspm build <source_dir>\n"; return; }
    std::string src = args[0];
    std::string out = src + ".spk";
    bool ok = spk::build(src, out);
    std::cout << (ok ? "✅ Built: " : "❌ Build failed: ") << out << "\n";
}

// ── sspm doctor ──────────────────────────────────────────────────────────
void cmd_doctor(const std::vector<std::string>& args) {
    std::cout << "Checking system health...\n";
    // TODO: Doctor::run()
    SSPM_INFO("doctor: system health check");
}

} // namespace sspm::cli
