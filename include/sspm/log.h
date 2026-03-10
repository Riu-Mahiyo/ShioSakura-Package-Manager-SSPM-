#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace sspm {

enum class LogLevel { Debug = 0, Info, Warn, Error };

// Thread-safe logger
// Writes to ~/.local/state/sspm/log/sspm.log
// Accessible via: sspm log / sspm log tail
class Logger {
public:
    static Logger& instance();

    void init(const std::string& log_path, LogLevel min_level = LogLevel::Info);
    void set_level(LogLevel level);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

    // sspm log — print recent N lines
    void print_recent(int lines = 50) const;

    // sspm log tail — follow log output (blocking)
    void tail() const;

private:
    Logger() = default;
    void write(LogLevel level, const std::string& msg);

    std::string log_path_;
    LogLevel min_level_ = LogLevel::Info;
    std::mutex mu_;
    std::ofstream file_;
};

// Convenience macros
#define SSPM_DEBUG(msg) sspm::Logger::instance().debug(msg)
#define SSPM_INFO(msg)  sspm::Logger::instance().info(msg)
#define SSPM_WARN(msg)  sspm::Logger::instance().warn(msg)
#define SSPM_ERROR(msg) sspm::Logger::instance().error(msg)

} // namespace sspm
