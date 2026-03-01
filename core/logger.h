#pragma once
// ═══════════════════════════════════════════════════════════
//  core/logger.h — Sky 统一日志
//  取自 v1.9 logger.h 精华，大幅精简
//  规则：只依赖 STL，不依赖本项目其他文件
// ═══════════════════════════════════════════════════════════
#include <string>
#include <iostream>

#define C_RESET  "\033[0m"
#define C_BOLD   "\033[1m"
#define C_RED    "\033[1;31m"
#define C_GREEN  "\033[1;32m"
#define C_YELLOW "\033[1;33m"
#define C_CYAN   "\033[1;36m"
#define C_GRAY   "\033[0;90m"

namespace SSPM {

class Logger {
public:
    // → 操作步骤
    static void step(const std::string& msg) {
        std::cout << C_CYAN "→ " C_RESET << msg << "\n";
    }
    // [OK]  成功
    static void ok(const std::string& msg) {
        std::cout << C_GREEN "[OK]  " C_RESET << msg << "\n";
    }
    // [WARN] 警告
    static void warn(const std::string& msg) {
        std::cout << C_YELLOW "[WARN] " C_RESET << msg << "\n";
    }
    // [ERR]  错误
    static void error(const std::string& msg) {
        std::cerr << C_RED "[ERR]  " C_RESET << msg << "\n";
    }
    // 普通信息（左对齐，带缩进）
    static void info(const std::string& msg) {
        std::cout << "       " << msg << "\n";
    }
    // [TXN]  事务日志
    static void txn(const std::string& msg) {
        std::cout << C_GRAY "[TXN]  " C_RESET << msg << "\n";
    }
    // debug 占位（--verbose 时展示，现在 no-op）
    static void debug(const std::string& /*msg*/) {}
    // v1 兼容别名
    static void success(const std::string& msg) { ok(msg); }
    static void warning(const std::string& msg) { warn(msg); }
    static void tip(const std::string& msg)     { info("💡 " + msg); }
};

} // namespace SSPM
