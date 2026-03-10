#include "sspm/database.h"
#include "sspm/cache.h"
#include "sspm/exec.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>
#include <curl/curl.h>
#include <openssl/opensslv.h>

namespace fs = std::filesystem;

namespace sspm::doctor {

struct Check {
    std::string name;
    bool passed;
    std::string detail;
};

static std::string get_home() {
    const char* home = getenv("HOME");
    return home ? std::string(home) : "/tmp";
}

static Check check_backends() {
    std::vector<std::string> tools = {
        "apt-get","pacman","dnf","zypper","brew","winget",
        "flatpak","snap","nix-env","pkg","apk","xbps-install"
    };
    std::string found;
    for (auto& t : tools)
        if (ExecEngine::exists(t))
            found += t + " ";
    return { "Backends", !found.empty(), found.empty() ? "No backends found" : "Found: "+found };
}

static Check check_permissions() {
    const std::string cache = get_home() + "/.cache/sspm";
    const std::string data  = get_home() + "/.local/share/sspm";
    std::error_code ec;
    fs::create_directories(cache, ec);
    fs::create_directories(data, ec);
    bool ok = !ec;
    return { "Permissions", ok, ok ? "Cache and data dirs writable" : ec.message() };
}

static Check check_network() {
    CURL* curl = curl_easy_init();
    if (!curl) return { "Network", false, "libcurl unavailable" };
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return { "Network", res == CURLE_OK,
             res == CURLE_OK ? "Connected" : "Error: " + std::string(curl_easy_strerror(res)) };
}

static Check check_database() {
    std::string db_path = get_home() + "/.local/share/sspm/sky.db";
    if (!fs::exists(db_path)) {
        SkyDB db(db_path);
        bool ok = db.open();
        return { "SkyDB", ok, ok ? "Database created OK" : "Failed to create database" };
    }
    SkyDB db(db_path);
    bool ok = db.open() && db.check_integrity();
    return { "SkyDB", ok, ok ? "Database integrity OK" : "Database integrity FAILED" };
}

static Check check_cache() {
    std::string cache_root = get_home() + "/.cache/sspm";
    CacheManager cache(cache_root);
    uint64_t sz = cache.total_size();
    return { "Cache", true, "Size: " + std::to_string(sz / 1024 / 1024) + " MB" };
}

static Check check_repos() {
    std::string db_path = get_home() + "/.local/share/sspm/sky.db";
    SkyDB db(db_path);
    if (!db.open()) return { "Repos", false, "Cannot open database" };
    auto repos = db.get_repos();
    if (repos.empty()) return { "Repos", true, "No repos configured yet" };
    return { "Repos", true, std::to_string(repos.size()) + " repos configured" };
}

static Check check_disk_space() {
    std::error_code ec;
    auto info = fs::space(get_home(), ec);
    if (ec) return { "Disk Space", false, "Could not check: " + ec.message() };
    
    uint64_t free_gb = info.available / (1024 * 1024 * 1024);
    bool ok = free_gb > 1;
    return { "Disk Space", ok, std::to_string(free_gb) + " GB available" };
}

static Check check_curl_version() {
    auto version_info = curl_version_info(CURLVERSION_NOW);
    if (!version_info) return { "libcurl", false, "Failed to get version" };
    
    std::string ver = version_info->version;
    bool ssl = version_info->features & CURL_VERSION_SSL;
    return { "libcurl", true, ver + (ssl ? " (SSL enabled)" : " (No SSL!)") };
}

static Check check_openssl() {
    return { "OpenSSL", true, OPENSSL_VERSION_TEXT };
}

static Check check_fuse() {
#ifdef SSPM_LINUX
    bool ok = fs::exists("/dev/fuse");
    return { "FUSE", ok, ok ? "Available (AppImage supported)" : "Missing (AppImage may fail)" };
#else
    return { "FUSE", true, "N/A on this platform" };
#endif
}

void run() {
    std::cout << "\n\033[1;35m🌸 SSPM Doctor\033[0m\n";
    std::cout << "══════════════════════════════════════════\n";

    std::vector<Check> checks = {
        check_backends(),
        check_permissions(),
        check_network(),
        check_database(),
        check_cache(),
        check_repos(),
        check_disk_space(),
        check_curl_version(),
        check_openssl(),
        check_fuse(),
    };

    int passed = 0;
    for (auto& c : checks) {
        std::string icon = c.passed ? "\033[32m✅\033[0m" : "\033[31m❌\033[0m";
        std::cout << icon << "  " << std::left;
        std::cout.width(16);
        std::cout << "\033[1m" << c.name << "\033[0m" << c.detail << "\n";
        if (c.passed) passed++;
    }

    std::cout << "══════════════════════════════════════════\n";
    if (passed == (int)checks.size())
        std::cout << "\033[32m✅ All " << passed << "/" << checks.size() << " checks passed\033[0m\n\n";
    else
        std::cout << "\033[33m⚠️  " << passed << "/" << checks.size() << " checks passed\033[0m\n\n";
}

} // namespace sspm::doctor
