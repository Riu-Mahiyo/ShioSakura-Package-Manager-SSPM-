#include "sspm/cli.h"
#include "sspm/log.h"
#include "sspm/transaction.h"
#include "sspm/database.h"
#include "sspm/resolver.h"
#include "sspm/backend_registry.h"
#include "sspm/validator.h"
#include <iostream>

namespace sspm::cli {

void cmd_install(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: sspm install <package> [--backend <n>] [--dry-run]\n";
        return;
    }

    std::string pkg_name;
    bool dry_run = false;
    std::string forced_backend;

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--dry-run")                          dry_run = true;
        else if (args[i] == "--backend" && i+1 < args.size()) forced_backend = args[++i];
        else if (args[i][0] != '-')                          pkg_name = args[i];
    }

    if (pkg_name.empty()) {
        std::cerr << "sspm install: no package name given\n";
        return;
    }

    auto val = InputValidator::pkg(pkg_name);
    if (!val) {
        std::cerr << "sspm install: invalid package name: " << val.reason << "\n";
        return;
    }

    if (dry_run) {
        std::cout << "[dry-run] Would install: " << pkg_name;
        if (!forced_backend.empty()) std::cout << " via " << forced_backend;
        std::cout << "\n";
        return;
    }

    SSPM_INFO("install: " + pkg_name);

    auto registry = BackendRegistry::create();
    
    // Load plugins (should probably be done once globally)
    std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
    sspm::plugin::PluginManager plugin_mgr(home + "/.local/share/sspm/plugins");
    plugin_mgr.load_all();
    registry.register_from_plugins(plugin_mgr);

    auto backends = registry.available_backends();
    Resolver resolver(backends);

    std::shared_ptr<Backend> backend;
    if (!forced_backend.empty()) {
        backend = registry.get(forced_backend);
        if (!backend || !backend->is_available()) {
            std::cerr << "sspm install: backend '" << forced_backend << "' not available\n";
            return;
        }
    } else {
        backend = resolver.resolve(pkg_name);
    }

    if (!backend) {
        std::cerr << "sspm install: could not resolve package '" << pkg_name << "'\n";
        return;
    }

    Transaction tx(DB_PATH);
    std::string tx_id = tx.begin();

    auto result = tx.execute([&]() -> Result {
        std::cout << "Installing " << pkg_name << " via " << backend->name() << "...\n";
        
        Package pkg;
        pkg.name = pkg_name;
        pkg.backend = backend->name();
        
        auto res = backend->install(pkg);
        if (!res.success) return res;

        SkyDB db(DB_PATH);
        if (db.open()) {
            db.insert_package(pkg);
            db.log_history("install", pkg);
        }
        
        return res;
    });

    if (result.success) {
        tx.commit(tx_id);
        std::cout << "✅ Installed: " << pkg_name << "\n";
    } else {
        tx.rollback(tx_id);
        std::cerr << "❌ Failed: " << result.error << "\n";
    }
}

} // namespace sspm::cli
