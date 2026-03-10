#include "sspm/cli.h"
#include "sspm/repo.h"
#include "sspm/log.h"
#include <iostream>

namespace sspm::cli {

void cmd_update(const std::vector<std::string>& args) {
    std::cout << "Syncing package databases...\n";
    SSPM_INFO("update: syncing all repos");
    RepoManager repo(DB_PATH, CACHE_PATH);
    bool ok = repo.sync_all();
    std::cout << (ok ? "✅ Done\n" : "⚠️  Some repos failed to sync\n");
}

void cmd_upgrade(const std::vector<std::string>& args) {
    std::string pkg;
    for (auto& a : args) if (a[0] != '-') pkg = a;

    if (!pkg.empty()) {
        std::cout << "Upgrading: " << pkg << "\n";
        SSPM_INFO("upgrade: " + pkg);
    } else {
        std::cout << "Upgrading all packages...\n";
        SSPM_INFO("upgrade: all");
    }
    // TODO: diff installed vs index versions, call backend->install() for newer
}

} // namespace sspm::cli
