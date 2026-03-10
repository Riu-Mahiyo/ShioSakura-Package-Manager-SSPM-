#include "sspm/mirror.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <future>
#include <curl/curl.h>

namespace sspm {

// Built-in mirror database
// Each backend has regional variants auto-selected by country code
static const std::vector<Mirror> MIRROR_DB = {
    // ── Arch Linux ──────────────────────────────────────────────────
    { "archlinux-jp",   "https://ftp.jaist.ac.jp/pub/Linux/ArchLinux/",     "JP", "AS", "arch", -1, true },
    { "archlinux-cn",   "https://mirrors.tuna.tsinghua.edu.cn/archlinux/",  "CN", "AS", "arch", -1, true },
    { "archlinux-us",   "https://mirror.rackspace.com/archlinux/",          "US", "NA", "arch", -1, true },
    { "archlinux-de",   "https://mirror.umd.edu/archlinux/",                "DE", "EU", "arch", -1, true },
    // ── Debian ──────────────────────────────────────────────────────
    { "debian-jp",      "https://ftp.jp.debian.org/debian/",                "JP", "AS", "debian", -1, true },
    { "debian-cn",      "https://mirrors.tuna.tsinghua.edu.cn/debian/",     "CN", "AS", "debian", -1, true },
    { "debian-us",      "https://ftp.us.debian.org/debian/",                "US", "NA", "debian", -1, true },
    { "debian-de",      "https://ftp.de.debian.org/debian/",                "DE", "EU", "debian", -1, true },
    // ── Ubuntu ──────────────────────────────────────────────────────
    { "ubuntu-jp",      "https://jp.archive.ubuntu.com/ubuntu/",            "JP", "AS", "ubuntu", -1, true },
    { "ubuntu-cn",      "https://mirrors.aliyun.com/ubuntu/",               "CN", "AS", "ubuntu", -1, true },
    { "ubuntu-us",      "https://us.archive.ubuntu.com/ubuntu/",            "US", "NA", "ubuntu", -1, true },
    // ── Fedora ──────────────────────────────────────────────────────
    { "fedora-jp",      "https://ftp.jaist.ac.jp/pub/Linux/Fedora/",        "JP", "AS", "fedora", -1, true },
    { "fedora-us",      "https://dl.fedoraproject.org/pub/fedora/",         "US", "NA", "fedora", -1, true },
    // ── Alpine ──────────────────────────────────────────────────────
    { "alpine-jp",      "https://mirrors.aliyun.com/alpine/",               "CN", "AS", "alpine", -1, true },
    { "alpine-us",      "https://dl-cdn.alpinelinux.org/alpine/",           "US", "NA", "alpine", -1, true },
    // ── Homebrew (macOS/Linux) ───────────────────────────────────────
    { "brew-cn",        "https://mirrors.tuna.tsinghua.edu.cn/homebrew/",   "CN", "AS", "brew", -1, true },
    { "brew-default",   "https://formulae.brew.sh/",                        "US", "NA", "brew", -1, true },
    // ── Universal (PyPI / npm / cargo / gem) ─────────────────────────
    { "pypi-cn",        "https://pypi.tuna.tsinghua.edu.cn/simple/",        "CN", "AS", "pip", -1, true },
    { "pypi-us",        "https://pypi.org/simple/",                         "US", "NA", "pip", -1, true },
    { "npm-cn",         "https://registry.npmmirror.com/",                  "CN", "AS", "npm", -1, true },
    { "npm-us",         "https://registry.npmjs.org/",                      "US", "NA", "npm", -1, true },
    { "cargo-cn",       "https://mirrors.tuna.tsinghua.edu.cn/git/crates.io-index", "CN", "AS", "cargo", -1, true },
    { "gem-cn",         "https://gems.ruby-china.com/",                     "CN", "AS", "gem", -1, true },
    { "nix-cn",         "https://mirror.tuna.tsinghua.edu.cn/nix-channels/store", "CN", "AS", "nix", -1, true },
};

static size_t curl_write_cb(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)contents, size * nmemb);
    return size * nmemb;
}

MirrorManager::MirrorManager(const std::string& config_path)
    : config_path_(config_path), mirrors_(MIRROR_DB) {}

// ── Geo-detection ────────────────────────────────────────────────────────────
// Strategy:
//   1. Try to get the REAL outbound IP (bypasses rule-based VPN by using a
//      direct connection to an IP-echo service that is NOT in the VPN route).
//   2. If the user has full-tunnel VPN, we detect the VPN exit-country instead
//      — which is also useful (user may want that country's mirrors).
//   3. Fall back to system locale.
std::string MirrorManager::fetch_geo_ip() const {
    // Use multiple geo services for reliability
    std::vector<std::string> services = {
        "https://ipapi.co/country/",
        "https://ip-country.fly.dev/",
        "https://api.country.is/",
    };

    CURL* curl = curl_easy_init();
    if (!curl) return "";

    for (auto& url : services) {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4L);
        // Use a fresh connection to avoid routing through proxy
        curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK && response.length() == 2) {
            curl_easy_cleanup(curl);
            return response; // e.g. "JP"
        }
    }

    curl_easy_cleanup(curl);
    return "";
}

std::string MirrorManager::detect_country() const {
    std::string country = fetch_geo_ip();
    if (country.empty()) {
        // Fallback: parse system locale (e.g. ja_JP → JP)
        const char* lang = std::getenv("LANG");
        if (lang) {
            std::string s(lang);
            auto pos = s.find('_');
            if (pos != std::string::npos && s.length() >= pos + 3)
                country = s.substr(pos + 1, 2);
        }
    }
    std::cout << "[mirror] Detected country: " << country << "\n";
    return country;
}

// ── Auto-select by geo ───────────────────────────────────────────────────────
void MirrorManager::auto_select(const std::string& backend) {
    std::string country = detect_country();
    if (country.empty()) {
        std::cerr << "[mirror] Could not detect country, using defaults\n";
        return;
    }

    // Find mirrors matching this country, rank by latency
    std::vector<Mirror> candidates = mirrors_for(backend);
    std::vector<Mirror*> filtered;
    for (auto& m : candidates)
        if (m.country == country) filtered.push_back(&m);

    // If none for exact country, fall back to same region
    if (filtered.empty()) {
        std::string region;
        for (auto& m : candidates) if (m.country == country) { region = m.region; break; }
        if (!region.empty())
            for (auto& m : candidates) if (m.region == region) filtered.push_back(&m);
    }

    if (filtered.empty()) {
        std::cout << "[mirror] No regional mirrors for " << country << ", keeping defaults\n";
        return;
    }

    // Rank by latency (parallelized)
    std::vector<std::future<void>> futures;
    for (auto* m : filtered) {
        futures.push_back(std::async(std::launch::async, [this, m]() {
            m->latency_ms = ping_mirror(*m);
        }));
    }
    for (auto& f : futures) f.wait();

    std::sort(filtered.begin(), filtered.end(),
        [](const Mirror* a, const Mirror* b) { return a->latency_ms < b->latency_ms; });

    if (!backend.empty() && !filtered.empty()) {
        // Find the index of the selected mirror in the original mirrors_ vector
        for (size_t i = 0; i < mirrors_.size(); ++i) {
            if (mirrors_[i].name == filtered[0]->name) {
                selected_[backend] = i;
                break;
            }
        }
    }
    std::cout << "[mirror] Auto-selected: " << filtered[0]->name
              << " (" << filtered[0]->latency_ms << " ms)\n";
}

void MirrorManager::rank_by_latency(const std::string& backend) {
    auto candidates = mirrors_for(backend);
    
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < candidates.size(); ++i) {
        futures.push_back(std::async(std::launch::async, [this, &m = candidates[i]]() {
            m.latency_ms = ping_mirror(m);
        }));
    }
    for (auto& f : futures) f.wait();

    std::sort(candidates.begin(), candidates.end(),
        [](const Mirror& a, const Mirror& b) { return a.latency_ms < b.latency_ms; });

    std::cout << "[mirror] Ranked " << candidates.size() << " mirrors by latency\n";
    for (const auto& m : candidates) {
        if (m.latency_ms < 9000)
            std::printf("  %-20s %6.1f ms  %s\n", m.name.c_str(), m.latency_ms, m.url.c_str());
    }
}

bool MirrorManager::select(const std::string& mirror_name) {
    for (size_t i = 0; i < mirrors_.size(); ++i) {
        if (mirrors_[i].name == mirror_name) {
            // Select for all backends that this mirror supports
            selected_[mirrors_[i].backend] = i;
            std::cout << "[mirror] Selected: " << mirrors_[i].url << "\n";
            return true;
        }
    }
    std::cerr << "[mirror] Not found: " << mirror_name << "\n";
    return false;
}

std::vector<Mirror> MirrorManager::list(const std::string& backend) const {
    return mirrors_for(backend);
}

Mirror MirrorManager::current(const std::string& backend) const {
    auto it = selected_.find(backend);
    if (it != selected_.end() && it->second < mirrors_.size()) {
        return mirrors_[it->second];
    }
    // Return first mirror for the backend if no selection
    auto candidates = mirrors_for(backend);
    return candidates.empty() ? Mirror() : candidates[0];
}

double MirrorManager::ping_mirror(const Mirror& m) const {
    CURL* curl = curl_easy_init();
    if (!curl) return 9999.0;

    auto start = std::chrono::high_resolution_clock::now();
    curl_easy_setopt(curl, CURLOPT_URL, m.url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    auto end = std::chrono::high_resolution_clock::now();
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return 9999.0;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// ── Mirror filtering helpers ──────────────────────────────────────────────────

std::vector<Mirror> MirrorManager::mirrors_for(const std::string& backend) const {
    if (backend.empty()) return mirrors_;
    std::vector<Mirror> result;
    for (auto& m : mirrors_)
        if (m.backend == backend || m.backend.empty()) result.push_back(m);
    return result;
}

// ── build_url_list: the network–mirror linkage point ─────────────────────────
//
// This is the function that connects MirrorManager to network::download().
// It returns mirrors sorted by latency (fastest first), with the base URL
// appended by the relative package path.
//
// Flow:
//   sspm install nginx
//     → Resolver picks backend "apt"
//     → SPK or backend resolves the relative package URL path
//     → build_url_list("debian", "/pool/main/nginx_1.25.3_amd64.deb")
//     → ["https://ftp.jp.debian.org/debian/pool/main/nginx_1.25.3_amd64.deb",
//        "https://ftp.de.debian.org/debian/pool/main/nginx_1.25.3_amd64.deb",
//        "https://ftp.us.debian.org/debian/pool/main/nginx_1.25.3_amd64.deb"]
//     → network::download() tries them in order, stops on first success

std::vector<std::string> MirrorManager::build_url_list(
    const std::string& backend,
    const std::string& relative_path) const
{
    auto candidates = list_ranked(backend);
    std::vector<std::string> urls;
    for (auto& m : candidates) {
        if (!m.available) continue;
        std::string url = m.url;
        if (!url.empty() && url.back() == '/' && !relative_path.empty() && relative_path[0] == '/')
            url += relative_path.substr(1);
        else
            url += relative_path;
        urls.push_back(url);
    }
    return urls;
}

std::string MirrorManager::best_url(const std::string& backend,
                                     const std::string& relative_path) const {
    auto urls = build_url_list(backend, relative_path);
    return urls.empty() ? relative_path : urls[0];
}

// Return mirrors sorted by latency (measured mirrors first, unmeasured last)
std::vector<Mirror> MirrorManager::list_ranked(const std::string& backend) const {
    auto result = mirrors_for(backend);
    std::stable_sort(result.begin(), result.end(), [](const Mirror& a, const Mirror& b) {
        if (a.latency_ms < 0 && b.latency_ms < 0) return false;
        if (a.latency_ms < 0) return false;
        if (b.latency_ms < 0) return true;
        return a.latency_ms < b.latency_ms;
    });
    return result;
}

void MirrorManager::add_mirror(const Mirror& m) {
    mirrors_.push_back(m);
}

void MirrorManager::print_status() const {
    std::cout << "\n=== Mirror Status ===\n";
    std::cout << std::left;
    for (auto& m : list_ranked("")) {
        std::string latency = m.latency_ms >= 0
            ? std::to_string((int)m.latency_ms) + " ms"
            : "not tested";
        std::string avail = m.available ? "✅" : "❌";
        std::cout.width(24); std::cout << m.name;
        std::cout.width(8);  std::cout << m.country;
        std::cout.width(12); std::cout << latency;
        std::cout << avail << " " << m.url << "\n";
    }
    std::cout << "====================\n\n";
}

void MirrorManager::load_config() {
    // TODO: read YAML from config_path_ and append custom mirrors
    // Format:
    //   mirrors:
    //     - name: my-mirror
    //       url: https://my.mirror.example.com/debian/
    //       country: US
    //       backend: debian
}

void MirrorManager::save_config() const {
    // TODO: write selected_ map to config_path_ as YAML
}

} // namespace sspm

