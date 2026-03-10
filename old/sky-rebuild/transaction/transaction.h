#pragma once
// ═══════════════════════════════════════════════════════════
//  transaction/transaction.h — 事务系统（v2.3）
//
//  v2.3 新增：
//    ✓ rollback 实装（调用 be->remove() 撤回已安装包）
//    ✓ 部分成功策略（KeepSuccess / AtomicAll）
//    ✓ 事务日志持久化（~/.local/share/sspm/txn.log）
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include <functional>

namespace SSPM {

enum class TxStatus {
    Idle,
    Active,
    Committed,
    Aborted,
};

// 部分成功策略
enum class TxPolicy {
    AtomicAll,    // 任何失败 → 全部回滚（默认）
    KeepSuccess,  // 保留已成功的，只报告失败项
};

struct TxOp {
    std::string type;    // "install", "remove", "upgrade"
    std::string backend;
    std::string pkg;
    bool done       = false;
    bool rolledBack = false;

    // rollback 函数（安装成功后设置：remove 该包）
    std::function<bool()> rollbackFn;
};

class Transaction {
public:
    explicit Transaction(const std::string& desc  = "",
                         TxPolicy           policy = TxPolicy::AtomicAll);
    ~Transaction();

    void begin();
    void addOp(const std::string& type,
               const std::string& backend,
               const std::string& pkg,
               std::function<bool()> rollbackFn = nullptr);
    void markDone(size_t opIdx);
    void commit();
    void abort(const std::string& reason = "");

    // 显式回滚所有 done 的操作
    int rollback();

    TxStatus status() const { return status_; }
    bool     active()  const { return status_ == TxStatus::Active; }

    // 安装锁
    static bool acquireLock();
    static void releaseLock();

private:
    std::string       desc_;
    TxPolicy          policy_;
    TxStatus          status_ = TxStatus::Idle;
    std::vector<TxOp> ops_;
    static int        lockFd_;

    void appendLog(const std::string& line);
};

} // namespace SSPM
