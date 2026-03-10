#pragma once
#include <string>
#include <cstdint>

namespace sspm {

// Manages ~/.cache/sspm
// Stores: downloaded packages, metadata, repo indexes
class CacheManager {
public:
    explicit CacheManager(const std::string& cache_root);

    // sspm cache size — total disk usage in bytes
    uint64_t total_size() const;

    // sspm cache clean — remove all cached files
    bool clean();

    // sspm cache prune — remove files older than max_age_days
    bool prune(int max_age_days = 30);

    // Store a downloaded file in cache
    bool store(const std::string& url, const std::string& data);

    // Check if a URL is cached
    bool has(const std::string& url) const;

    // Retrieve cached file path
    std::string get_path(const std::string& url) const;

private:
    std::string root_;
    std::string url_to_filename(const std::string& url) const;
};

} // namespace sspm
