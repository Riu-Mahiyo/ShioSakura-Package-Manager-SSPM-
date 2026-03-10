#pragma once
#include <string>
#include <vector>

namespace sspm {

struct RepoEntry {
    std::string name;
    std::string url;
    bool enabled;
    std::string last_sync;
};

// Manages SSPM package repositories.
// Supports official, third-party, and local repos.
// Repo format: repo.json + packages index + ed25519 signature
class RepoManager {
public:
    explicit RepoManager(const std::string& db_path,
                         const std::string& cache_path);

    // sspm repo add <url>
    bool add(const std::string& name, const std::string& url);

    // sspm repo remove <name>
    bool remove(const std::string& name);

    // sspm repo sync — fetch latest index from all repos
    bool sync_all();
    bool sync(const std::string& name);

    // sspm repo list
    std::vector<RepoEntry> list() const;

    // Verify repo signature (ed25519)
    bool verify_repo(const std::string& name) const;

private:
    std::string db_path_;
    std::string cache_path_;

    bool fetch_index(const RepoEntry& repo);
    bool verify_signature(const std::string& index_path,
                          const std::string& sig_path,
                          const std::string& pubkey) const;
};

} // namespace sspm
