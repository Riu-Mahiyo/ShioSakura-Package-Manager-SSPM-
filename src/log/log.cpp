#include "sspm/log.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;
namespace sspm {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::init(const std::string& log_path, LogLevel min_level) {
    std::lock_guard<std::mutex> lock(mu_);
    log_path_ = log_path;
    min_level_ = min_level;
    fs::create_directories(fs::path(log_path).parent_path());
    file_.open(log_path, std::ios::app);
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mu_);
    min_level_ = level;
}

static std::string level_str(LogLevel l) {
    switch (l) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
    }
    return "?";
}

static std::string now_str() {
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::write(LogLevel level, const std::string& msg) {
    if (level < min_level_) return;
    
    std::string color;
    switch (level) {
        case LogLevel::Debug: color = "\033[36m"; break; // Cyan
        case LogLevel::Info:  color = "\033[32m"; break; // Green
        case LogLevel::Warn:  color = "\033[33m"; break; // Yellow
        case LogLevel::Error: color = "\033[31m"; break; // Red
    }
    std::string reset = "\033[0m";

    std::lock_guard<std::mutex> lock(mu_);
    std::string log_line = "[" + now_str() + "] [" + level_str(level) + "] " + msg + "\n";
    if (file_.is_open()) file_ << log_line << std::flush;
    
    // Print to stderr/stdout with colors for interactive CLI
    if (level >= LogLevel::Warn) {
        std::cerr << color << log_line << reset;
    } else if (level == LogLevel::Info) {
        std::cout << color << "🌸 " << msg << reset << "\n";
    }
}

void Logger::debug(const std::string& msg) { write(LogLevel::Debug, msg); }
void Logger::info(const std::string& msg)  { write(LogLevel::Info,  msg); }
void Logger::warn(const std::string& msg)  { write(LogLevel::Warn,  msg); }
void Logger::error(const std::string& msg) { write(LogLevel::Error, msg); }

void Logger::print_recent(int lines) const {
    if (!fs::exists(log_path_)) {
        std::cout << "[log] No log file yet: " << log_path_ << "\n";
        return;
    }
    std::ifstream f(log_path_);
    std::vector<std::string> all;
    std::string line;
    while (std::getline(f, line)) all.push_back(line);
    int start = std::max(0, (int)all.size() - lines);
    for (int i = start; i < (int)all.size(); i++)
        std::cout << all[i] << "\n";
}

void Logger::tail() const {
    std::cout << "[log] Tailing: " << log_path_ << " (Ctrl+C to stop)\n";
    std::ifstream f(log_path_);
    f.seekg(0, std::ios::end);
    while (true) {
        std::string line;
        if (std::getline(f, line)) {
            std::cout << line << "\n";
        } else {
            f.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
}

} // namespace sspm
