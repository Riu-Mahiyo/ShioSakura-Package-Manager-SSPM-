#include "sspm/repo.h"
#include "sspm/database.h"
#include <iostream>
#include <fstream>
#include <curl/curl.h>

namespace sspm {

RepoManager::RepoManager(const std::string& db_path, const std::string& cache_path)
    : db_path_(db_path), cache_path_(cache_path) {}

bool RepoManager::add(const std::string& name, const std::string& url) {
    SkyDB db(db_path_);
    if (!db.open()) return false;
    bool ok = db.add_repo(name, url);
    if (ok) {
        std::cout << "[repo] Added: " << name << " → " << url << "\n";
        sync(name);
    }
    return ok;
}

bool RepoManager::remove(const std::string& name) {
    SkyDB db(db_path_);
    if (!db.open()) return false;
    bool ok = db.remove_repo(name);
    if (ok) std::cout << "[repo] Removed: " << name << "\n";
    return ok;
}

bool RepoManager::sync_all() {
    SkyDB db(db_path_);
    if (!db.open()) return false;
    auto repos = db.get_repos();
    
    db.begin_transaction(); // Performance: Batch sync in one transaction
    bool all_ok = true;
    for (auto& [name, url] : repos) {
        RepoEntry r{ name, url, true, "" };
        if (!fetch_index(r)) {
            std::cerr << "[repo] Sync failed: " << name << "\n";
            all_ok = false;
        } else {
            std::cout << "[repo] Synced: " << name << "\n";
        }
    }
    db.commit_transaction();
    return all_ok;
}

bool RepoManager::sync(const std::string& name) {
    SkyDB db(db_path_);
    if (!db.open()) return false;
    auto repos = db.get_repos();
    for (auto& [n, url] : repos) {
        if (n == name) {
            RepoEntry r{ n, url, true, "" };
            return fetch_index(r);
        }
    }
    return false;
}

std::vector<RepoEntry> RepoManager::list() const {
    SkyDB db(db_path_);
    if (!const_cast<SkyDB&>(db).open()) return {};
    auto repos = db.get_repos();
    std::vector<RepoEntry> result;
    for (auto& [n, url] : repos)
        result.push_back({ n, url, true, "" });
    return result;
}

bool RepoManager::fetch_index(const RepoEntry& repo) {
    // Use libcurl to download repo.json from repo.url
    std::string index_url = repo.url + "/repo.json";
    std::string out_path = cache_path_ + "/repos/" + repo.name + ".json";

    // TODO: full curl download with progress, resume, and signature check
    std::cout << "[repo] Fetching index: " << index_url << "\n";
    return true; // placeholder
}

bool RepoManager::verify_signature(const std::string& index_path,
                                   const std::string& sig_path,
                                   const std::string& pubkey) const {
    // TODO: verify ed25519 signature using OpenSSL
    return true; // placeholder
}

} // namespace sspm
