#include "sspm/database.h"
#include <iostream>
#include <stdexcept>

namespace sspm {

SkyDB::SkyDB(const std::string& path) : path_(path) {}

SkyDB::~SkyDB() {
    for (auto& pair : stmt_cache_) {
        sqlite3_finalize(pair.second);
    }
    if (db_) sqlite3_close(db_);
}

sqlite3_stmt* SkyDB::get_cached_stmt(const std::string& sql) const {
    auto it = stmt_cache_.find(sql);
    if (it != stmt_cache_.end()) {
        sqlite3_reset(it->second);
        sqlite3_clear_bindings(it->second);
        return it->second;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return nullptr;

    stmt_cache_[sql] = stmt;
    return stmt;
}

bool SkyDB::begin_transaction() {
    char* err = nullptr;
    return sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &err) == SQLITE_OK;
}

bool SkyDB::commit_transaction() {
    char* err = nullptr;
    return sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err) == SQLITE_OK;
}

bool SkyDB::rollback_transaction() {
    char* err = nullptr;
    return sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &err) == SQLITE_OK;
}

bool SkyDB::open() {
    int rc = sqlite3_open(path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[skydb] Cannot open database: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    // Performance Optimization: PRAGMAs for speed and reliability
    sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA cache_size = -2000;", nullptr, nullptr, nullptr); // 2MB cache
    sqlite3_exec(db_, "PRAGMA busy_timeout = 5000;", nullptr, nullptr, nullptr);

    return init_schema();
}

bool SkyDB::init_schema() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS packages (
            name        TEXT PRIMARY KEY,
            version     TEXT NOT NULL,
            description TEXT,
            backend     TEXT NOT NULL,
            repo        TEXT,
            platform    TEXT,
            hash        TEXT,
            install_time TEXT,
            desktop_reg  INTEGER DEFAULT 0,
            mime_reg     INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS history (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            action      TEXT NOT NULL,
            pkg_name    TEXT NOT NULL,
            pkg_version TEXT,
            backend     TEXT,
            timestamp   TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS transactions (
            tx_id       TEXT PRIMARY KEY,
            action      TEXT,
            pkg_name    TEXT,
            status      TEXT NOT NULL,
            created_at  TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS repos (
            name        TEXT PRIMARY KEY,
            url         TEXT NOT NULL,
            enabled     INTEGER DEFAULT 1,
            last_sync   TEXT
        );

        CREATE TABLE IF NOT EXISTS profiles (
            name        TEXT PRIMARY KEY,
            packages    TEXT,
            created_at  TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_history_pkg ON history(pkg_name);
        CREATE INDEX IF NOT EXISTS idx_transactions_pkg ON transactions(pkg_name);
        CREATE INDEX IF NOT EXISTS idx_history_time ON history(timestamp);
    )";

    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "[skydb] Schema init failed: " << err << "\n";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool SkyDB::insert_transaction(const TxEntry& entry) {
    const char* sql =
        "INSERT OR REPLACE INTO transactions(tx_id, action, pkg_name, status, created_at)"
        " VALUES(?,?,?,?,?);";
    sqlite3_stmt* stmt = get_cached_stmt(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, entry.id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.action.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry.package.name.c_str(), -1, SQLITE_STATIC);

    std::string status_str;
    switch (entry.status) {
        case TxStatus::Pending:    status_str = "pending"; break;
        case TxStatus::Committed:  status_str = "committed"; break;
        case TxStatus::RolledBack: status_str = "rolled_back"; break;
        case TxStatus::Failed:     status_str = "failed"; break;
    }
    sqlite3_bind_text(stmt, 4, status_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, entry.timestamp.c_str(), -1, SQLITE_STATIC);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

std::vector<TxEntry> SkyDB::get_transaction_history(int limit) const {
    std::vector<TxEntry> result;
    const char* sql = "SELECT tx_id, action, pkg_name, status, created_at FROM transactions"
                      " ORDER BY created_at DESC LIMIT ?;";
    sqlite3_stmt* stmt = get_cached_stmt(sql);
    if (!stmt) return {};
    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TxEntry e;
        e.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        e.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        e.package.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        std::string s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (s == "pending") e.status = TxStatus::Pending;
        else if (s == "committed") e.status = TxStatus::Committed;
        else if (s == "rolled_back") e.status = TxStatus::RolledBack;
        else e.status = TxStatus::Failed;

        e.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        result.push_back(e);
    }
    return result;
}

bool SkyDB::insert_package(const Package& pkg) {
    const char* sql =
        "INSERT OR REPLACE INTO packages(name,version,description,backend,repo,platform,hash,install_time,desktop_reg,mime_reg)"
        " VALUES(?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt = get_cached_stmt(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, pkg.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pkg.version.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pkg.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, pkg.backend.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, pkg.repo.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, pkg.platform.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, pkg.hash.c_str(), -1, SQLITE_STATIC);
    
    std::string itime = pkg.install_time.empty() ? "datetime('now')" : pkg.install_time;
    sqlite3_bind_text(stmt, 8, itime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, pkg.desktop_registered ? 1 : 0);
    sqlite3_bind_int(stmt, 10, pkg.mime_registered ? 1 : 0);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

bool SkyDB::remove_package(const std::string& name) {
    const char* sql = "DELETE FROM packages WHERE name=?;";
    sqlite3_stmt* stmt = get_cached_stmt(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    return sqlite3_step(stmt) == SQLITE_DONE;
}

std::vector<Package> SkyDB::get_installed() const {
    std::vector<Package> result;
    const char* sql = "SELECT name,version,description,backend,repo,platform,hash,install_time,desktop_reg,mime_reg FROM packages;";
    sqlite3_stmt* stmt = get_cached_stmt(sql);
    if (!stmt) return {};

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Package p;
        p.name        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        p.version     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        auto desc = sqlite3_column_text(stmt, 2);
        if (desc) p.description = reinterpret_cast<const char*>(desc);
        p.backend     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        auto repo = sqlite3_column_text(stmt, 4);
        if (repo) p.repo = reinterpret_cast<const char*>(repo);
        auto platform = sqlite3_column_text(stmt, 5);
        if (platform) p.platform = reinterpret_cast<const char*>(platform);
        auto hash = sqlite3_column_text(stmt, 6);
        if (hash) p.hash = reinterpret_cast<const char*>(hash);
        auto itime = sqlite3_column_text(stmt, 7);
        if (itime) p.install_time = reinterpret_cast<const char*>(itime);
        p.desktop_registered = sqlite3_column_int(stmt, 8) != 0;
        p.mime_registered    = sqlite3_column_int(stmt, 9) != 0;
        result.push_back(p);
    }
    return result;
}

std::optional<Package> SkyDB::get_package(const std::string& name) const {
    const char* sql = "SELECT name,version,description,backend,repo,platform,hash,install_time,desktop_reg,mime_reg FROM packages WHERE name=?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Package p;
        p.name        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        p.version     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        auto desc = sqlite3_column_text(stmt, 2);
        if (desc) p.description = reinterpret_cast<const char*>(desc);
        p.backend     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        auto repo = sqlite3_column_text(stmt, 4);
        if (repo) p.repo = reinterpret_cast<const char*>(repo);
        auto platform = sqlite3_column_text(stmt, 5);
        if (platform) p.platform = reinterpret_cast<const char*>(platform);
        auto hash = sqlite3_column_text(stmt, 6);
        if (hash) p.hash = reinterpret_cast<const char*>(hash);
        auto itime = sqlite3_column_text(stmt, 7);
        if (itime) p.install_time = reinterpret_cast<const char*>(itime);
        p.desktop_registered = sqlite3_column_int(stmt, 8) != 0;
        p.mime_registered    = sqlite3_column_int(stmt, 9) != 0;
        sqlite3_finalize(stmt);
        return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool SkyDB::log_history(const std::string& action, const Package& pkg) {
    const char* sql =
        "INSERT INTO history(action,pkg_name,pkg_version,backend,timestamp)"
        " VALUES(?,?,?,?,datetime('now'));";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pkg.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pkg.version.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, pkg.backend.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<std::string> SkyDB::get_history(int limit) const {
    std::vector<std::string> result;
    const char* sql = "SELECT timestamp,action,pkg_name,backend FROM history ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string row =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))) + "  " +
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))) + "  " +
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))) + "  [" +
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))) + "]";
        result.push_back(row);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool SkyDB::add_repo(const std::string& name, const std::string& url) {
    const char* sql = "INSERT OR REPLACE INTO repos(name,url) VALUES(?,?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SkyDB::remove_repo(const std::string& name) {
    const char* sql = "DELETE FROM repos WHERE name=?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<std::pair<std::string, std::string> > SkyDB::get_repos() const {
    std::vector<std::pair<std::string, std::string> > result;
    const char* sql = "SELECT name,url FROM repos WHERE enabled=1;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.emplace_back(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))
        );
    }
    sqlite3_finalize(stmt);
    return result;
}

bool SkyDB::check_integrity() const {
    const char* sql = "PRAGMA integrity_check;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string res = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        ok = (res == "ok");
    }
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace sspm
