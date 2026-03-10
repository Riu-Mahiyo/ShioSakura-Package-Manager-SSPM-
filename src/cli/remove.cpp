#include "sspm/cli.h"
#include "sspm/log.h"
#include "sspm/transaction.h"
#include "sspm/database.h"
#include <iostream>

namespace sspm::cli {

void cmd_remove(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: sspm remove <package> [--purge]\n";
        return;
    }

    std::string pkg_name;
    bool purge = false;

    for (auto& a : args) {
        if (a == "--purge") purge = true;
        else if (a[0] != '-') pkg_name = a;
    }

    if (pkg_name.empty()) {
        std::cerr << "sspm remove: no package name given\n";
        return;
    }

    SSPM_INFO("remove: " + pkg_name + (purge ? " --purge" : ""));

    SkyDB db(DB_PATH);
    if (!db.open()) { std::cerr << "Cannot open database\n"; return; }

    auto pkg = db.get_package(pkg_name);
    if (!pkg) {
        std::cerr << "Package not installed: " << pkg_name << "\n";
        return;
    }

    Transaction tx(DB_PATH);
    std::string tx_id = tx.begin();

    auto result = tx.execute([&]() -> Result {
        std::cout << "Removing: " << pkg_name << " [" << pkg->backend << "]\n";
        // TODO: backend->remove(*pkg)
        return { true, "Removed " + pkg_name, "" };
    });

    if (result.success) {
        db.remove_package(pkg_name);
        db.log_history("remove", *pkg);
        tx.commit(tx_id);
        std::cout << "✅ Removed: " << pkg_name << "\n";
    } else {
        tx.rollback(tx_id);
        std::cerr << "❌ Failed: " << result.error << "\n";
    }
}

} // namespace sspm::cli
