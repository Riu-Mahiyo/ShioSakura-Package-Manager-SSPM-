#include "sspm/cache.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <functional>

namespace fs = std::filesystem;
namespace sspm {

CacheManager::CacheManager(const std::string& cache_root) : root_(cache_root) {
    fs::create_directories(root_ + "/packages");
    fs::create_directories(root_ + "/metadata");
    fs::create_directories(root_ + "/repos");
}

uint64_t CacheManager::total_size() const {
    uint64_t total = 0;
    std::error_code ec;
    if (!fs::exists(root_, ec)) return 0;
    
    for (auto& entry : fs::recursive_directory_iterator(root_, ec)) {
        if (ec) break;
        if (entry.is_regular_file(ec))
            total += entry.file_size(ec);
    }
    return total;
}

bool CacheManager::clean() {
    std::error_code ec;
    fs::remove_all(root_, ec);
    if (ec) {
        std::cerr << "[cache] Failed to remove root: " << ec.message() << "\n";
    }
    fs::create_directories(root_ + "/packages", ec);
    fs::create_directories(root_ + "/metadata", ec);
    fs::create_directories(root_ + "/repos", ec);
    if (!ec) std::cout << "[cache] Cleaned: " << root_ << "\n";
    return !ec;
}

bool CacheManager::prune(int max_age_days) {
    std::error_code ec;
    if (!fs::exists(root_, ec)) return true;

    auto cutoff = fs::file_time_type::clock::now()
        - std::chrono::hours(24 * max_age_days);
    int removed = 0;
    for (auto& entry : fs::recursive_directory_iterator(root_, ec)) {
        if (ec) break;
        if (entry.is_regular_file(ec)) {
            if (entry.last_write_time(ec) < cutoff) {
                fs::remove(entry.path(), ec);
                if (!ec) removed++;
            }
        }
    }
    std::cout << "[cache] Pruned " << removed << " old files\n";
    return true;
}

bool CacheManager::store(const std::string& url, const std::string& data) {
    std::string path = get_path(url);
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(data.data(), data.size());
    return true;
}

bool CacheManager::has(const std::string& url) const {
    return fs::exists(get_path(url));
}

std::string CacheManager::get_path(const std::string& url) const {
    return root_ + "/packages/" + url_to_filename(url);
}

std::string CacheManager::url_to_filename(const std::string& url) const {
    // Simple hash-based filename to avoid path issues
    size_t h = std::hash<std::string>{}(url);
    return std::to_string(h) + ".pkg";
}

} // namespace sspm
