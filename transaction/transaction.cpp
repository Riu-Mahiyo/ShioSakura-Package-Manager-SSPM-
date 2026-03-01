// ═══════════════════════════════════════════════════════════
//  transaction/transaction.cpp — 事务系统（v2.3）
//  新增：rollback 实装、事务日志持久化
// ═══════════════════════════════════════════════════════════
#include "transaction.h"
#include "../core/logger.h"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

namespace SSPM {
namespace fs = std::filesystem;

int Transaction::lockFd_ = -1;

static std::string lockPath() {
    const char* xdg = getenv("XDG_RUNTIME_DIR");
    if (xdg && *xdg) return std::string(xdg) + "/sspm.lock";
    return "/tmp/sspm.lock";
}

static std::string logPath() {
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base;
    if (xdg && *xdg) base = std::string(xdg) + "/sky";
    else {
        const char* home = getenv("HOME");
        base = std::string(home ? home : "/tmp") + "/.local/share/sky";
    }
    return base + "/txn.log";
}

Transaction::Transaction(const std::string& desc, TxPolicy policy)
    : desc_(desc), policy_(policy) {}

Transaction::~Transaction() {
    if (status_ == TxStatus::Active) {
        Logger::warn("[TXN] 析构时事务仍 Active，自动 abort: " + desc_);
        abort("析构保护");
    }
}

void Transaction::appendLog(const std::string& line) {
    std::string path = logPath();
    try { fs::create_directories(fs::path(path).parent_path()); }
    catch (...) { return; }

    std::ofstream f(path, std::ios::app);
    if (!f) return;

    auto now = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));
    f << ts << " " << line << "\n";
}

void Transaction::begin() {
    Logger::txn("begin  — " + desc_);
    appendLog("BEGIN " + desc_);
    status_ = TxStatus::Active;
    ops_.clear();
}

void Transaction::addOp(const std::string& type,
                         const std::string& backend,
                         const std::string& pkg,
                         std::function<bool()> rollbackFn)
{
    ops_.push_back({type, backend, pkg, false, false, rollbackFn});
    Logger::txn("  op: " + type + " [" + backend + "] " + pkg);
}

void Transaction::markDone(size_t idx) {
    if (idx < ops_.size()) {
        ops_[idx].done = true;
        appendLog("OP_DONE " + ops_[idx].type + " " + ops_[idx].pkg);
    }
}

void Transaction::commit() {
    if (status_ != TxStatus::Active) {
        Logger::warn("[TXN] commit called but not Active");
        return;
    }
    Logger::txn("commit — " + desc_);
    appendLog("COMMIT " + desc_);
    status_ = TxStatus::Committed;
}

int Transaction::rollback() {
    int failed = 0;
    // 倒序回滚（LIFO）
    for (int i = (int)ops_.size() - 1; i >= 0; --i) {
        auto& op = ops_[i];
        if (!op.done || op.rolledBack) continue;

        Logger::txn("  rollback: " + op.type + " [" + op.backend + "] " + op.pkg);
        appendLog("ROLLBACK_OP " + op.type + " " + op.pkg);

        bool ok = true;
        if (op.rollbackFn) {
            ok = op.rollbackFn();
        }
        // 如果没有 rollbackFn，只能报告无法回滚
        if (ok) {
            op.rolledBack = true;
            Logger::ok("已回滚: " + op.pkg);
        } else {
            Logger::error("回滚失败: " + op.pkg + " (需要手动处理)");
            failed++;
        }
    }
    return failed;
}

void Transaction::abort(const std::string& reason) {
    if (status_ != TxStatus::Active) return;
    Logger::txn("abort  — " + desc_ +
                (reason.empty() ? "" : " (" + reason + ")"));
    appendLog("ABORT " + desc_ + (reason.empty() ? "" : " reason=" + reason));

    if (policy_ == TxPolicy::AtomicAll) {
        int failed = rollback();
        if (failed > 0) {
            Logger::error("部分回滚失败，请手动检查系统状态");
            Logger::info("事务日志: " + logPath());
        }
    } else {
        Logger::info("策略 KeepSuccess：保留已成功的操作，跳过回滚");
    }

    status_ = TxStatus::Aborted;
}

bool Transaction::acquireLock() {
    std::string path = lockPath();
    try { fs::create_directories(fs::path(path).parent_path()); }
    catch (...) {}

    lockFd_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lockFd_ < 0) {
        Logger::error("[TXN] 无法创建锁文件: " + path);
        return false;
    }

    // 检查陈旧锁（进程已死）
    char pidbuf[32] = {};
    ::read(lockFd_, pidbuf, sizeof(pidbuf) - 1);
    if (pidbuf[0]) {
        pid_t pid = (pid_t)atoi(pidbuf);
        if (pid > 1 && ::kill(pid, 0) != 0) {
            // 进程已死，清理陈旧锁
            Logger::warn("[TXN] 清理陈旧锁 (PID " + std::to_string(pid) + " 已不存在)");
            ::lseek(lockFd_, 0, SEEK_SET);
            ::ftruncate(lockFd_, 0);
        }
    }

    if (::flock(lockFd_, LOCK_EX | LOCK_NB) < 0) {
        // 读取持有者 PID
        char buf[32] = {};
        ::pread(lockFd_, buf, sizeof(buf) - 1, 0);
        std::string holder = buf[0] ? std::string("PID ") + buf : "unknown";
        Logger::error("[TXN] 另一个 sky 进程正在运行 (" + holder + ")，请稍后再试");
        ::close(lockFd_);
        lockFd_ = -1;
        return false;
    }

    // 写入当前 PID
    char pidbuf2[32];
    snprintf(pidbuf2, sizeof(pidbuf2), "%d", (int)::getpid());
    ::lseek(lockFd_, 0, SEEK_SET);
    ::ftruncate(lockFd_, 0);
    ::write(lockFd_, pidbuf2, strlen(pidbuf2));

    return true;
}

void Transaction::releaseLock() {
    if (lockFd_ >= 0) {
        ::flock(lockFd_, LOCK_UN);
        ::close(lockFd_);
        lockFd_ = -1;
    }
}

} // namespace SSPM
