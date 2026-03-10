#pragma once
#include "backend.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <sqlite3.h>
#include <optional>
#include <map>

namespace sspm {

// SkyDB: SQLite-backed local state for SSPM
// Tables: packages, history, transactions, repos, profiles
class SkyDB {
public:
    explicit SkyDB(const std::string& path);
    ~SkyDB();

    bool open();
    bool init_schema();

    // Packages
    bool insert_package(const Package& pkg);
    bool remove_package(const std::string& name);
    std::vector<Package> get_installed() const;
    std::optional<Package> get_package(const std::string& name) const;

    // History
    bool log_history(const std::string& action, const Package& pkg);
    std::vector<std::string> get_history(int limit = 100) const;

    // Transactions
    bool insert_transaction(const TxEntry& entry);
    std::vector<TxEntry> get_transaction_history(int limit = 50) const;

    // Performance & Safety
    bool begin_transaction();
    bool commit_transaction();
    bool rollback_transaction();

    // Repos
    bool add_repo(const std::string& name, const std::string& url);
    bool remove_repo(const std::string& name);
    std::vector<std::pair<std::string, std::string> > get_repos() const;

    // Integrity
    bool check_integrity() const;

private:
    std::string path_;
    sqlite3* db_ = nullptr;
    mutable std::map<std::string, sqlite3_stmt*> stmt_cache_;

    sqlite3_stmt* get_cached_stmt(const std::string& sql) const;
};

} // namespace sspm
