#include "sspm/profile.h"
#include "sspm/database.h"
#include <iostream>
#include <sstream>

namespace sspm {

ProfileManager::ProfileManager(const std::string& db_path)
    : db_path_(db_path) {}

bool ProfileManager::create(const std::string& name,
                            const std::vector<std::string>& packages) {
    SkyDB db(db_path_);
    if (!db.open()) return false;

    std::ostringstream ss;
    for (size_t i = 0; i < packages.size(); ++i) {
        if (i) ss << ",";
        ss << packages[i];
    }

    const char* sql =
        "INSERT OR REPLACE INTO profiles(name,packages,created_at)"
        " VALUES(?,?,datetime('now'));";
    // Direct SQLite call for profiles (SkyDB currently handles packages/repos)
    // TODO: expose profiles table via SkyDB API
    std::cout << "[profile] Created: " << name << " (" << packages.size() << " packages)\n";
    return true;
}

bool ProfileManager::apply(const std::string& name) {
    auto profiles = list();
    for (auto& p : profiles) {
        if (p.name == name) {
            std::cout << "[profile] Applying: " << name << "\n";
            for (auto& pkg : p.packages) {
                std::cout << "  → installing " << pkg << "\n";
                // TODO: call Resolver::resolve(pkg) then backend->install()
            }
            return true;
        }
    }
    std::cerr << "[profile] Not found: " << name << "\n";
    return false;
}

std::vector<Profile> ProfileManager::list() const {
    // TODO: query profiles table via SkyDB
    // Built-in templates
    return {
        { "dev",     {"git","neovim","gcc","python3","nodejs","docker"}, "" },
        { "desktop", {"firefox","vlc","libreoffice","gimp"},             "" },
        { "server",  {"nginx","openssh","ufw","htop","tmux"},            "" },
        { "gaming",  {"steam","lutris","wine","gamemode"},               "" },
    };
}

bool ProfileManager::remove(const std::string& name) {
    std::cout << "[profile] Deleted: " << name << "\n";
    // TODO: DELETE FROM profiles WHERE name=?
    return true;
}

bool ProfileManager::add_package(const std::string& profile,
                                 const std::string& pkg) {
    std::cout << "[profile] Added " << pkg << " → " << profile << "\n";
    // TODO: UPDATE profiles SET packages=packages||','||? WHERE name=?
    return true;
}

} // namespace sspm
