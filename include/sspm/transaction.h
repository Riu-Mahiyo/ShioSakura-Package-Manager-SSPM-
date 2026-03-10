#pragma once
#include "backend.h"
#include <string>
#include <vector>
#include <functional>

namespace sspm {

enum class TxStatus { Pending, Committed, RolledBack, Failed };

struct TxEntry {
    std::string id;
    std::string action;     // "install" | "remove"
    Package package;
    TxStatus status;
    std::string timestamp;
};

// Atomic transaction: begin → execute → commit / rollback
class Transaction {
public:
    explicit Transaction(const std::string& db_path);

    std::string begin();                          // Returns tx_id
    Result execute(std::function<Result()> op);   // Run op inside tx
    Result commit(const std::string& tx_id);
    Result rollback(const std::string& tx_id);

    std::vector<TxEntry> history(int limit = 50) const;

private:
    std::string db_path_;
    void log_entry(const TxEntry& entry);
};

} // namespace sspm
