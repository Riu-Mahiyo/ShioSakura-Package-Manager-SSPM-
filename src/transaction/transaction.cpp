#include "sspm/transaction.h"
#include "sspm/database.h"
#include <sqlite3.h>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>

namespace sspm {

static std::string now_iso() {
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

static std::string gen_id() {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    std::string id = "tx-";
    for (int i = 0; i < 8; i++) id += "0123456789abcdef"[dist(rng)];
    return id;
}

Transaction::Transaction(const std::string& db_path)
    : db_path_(db_path) {}

// Begin a new transaction — returns a unique tx_id
std::string Transaction::begin() {
    std::string id = gen_id();
    TxEntry entry{ id, "begin", {}, TxStatus::Pending, now_iso() };
    log_entry(entry);
    std::cout << "[tx] Begin transaction: " << id << "\n";
    return id;
}

// Execute an operation inside a transaction context
// On failure, sets status to Failed (caller should rollback)
Result Transaction::execute(std::function<Result()> op) {
    try {
        return op();
    } catch (const std::exception& e) {
        return { false, "", std::string("Exception: ") + e.what() };
    }
}

// Commit: mark transaction as successful
Result Transaction::commit(const std::string& tx_id) {
    TxEntry entry{ tx_id, "commit", {}, TxStatus::Committed, now_iso() };
    log_entry(entry);
    std::cout << "[tx] Committed: " << tx_id << "\n";
    return { true, "Transaction committed", "" };
}

// Rollback: mark transaction as rolled back, attempt undo
Result Transaction::rollback(const std::string& tx_id) {
    TxEntry entry{ tx_id, "rollback", {}, TxStatus::RolledBack, now_iso() };
    log_entry(entry);
    std::cout << "[tx] Rolled back: " << tx_id << "\n";
    // TODO: replay undo log for crash recovery
    return { true, "Transaction rolled back", "" };
}

std::vector<TxEntry> Transaction::history(int limit) const {
    SkyDB db(db_path_);
    if (!db.open()) return {};
    return db.get_transaction_history(limit);
}

void Transaction::log_entry(const TxEntry& entry) {
    SkyDB db(db_path_);
    if (db.open()) {
        db.insert_transaction(entry);
    }
}

} // namespace sspm
