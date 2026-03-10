#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace sspm {

struct Mirror {
    std::string name;
    std::string url;        // base URL, e.g. "https://ftp.jaist.ac.jp/pub/Linux/ArchLinux/"
    std::string country;    // ISO 3166-1 alpha-2, e.g. "JP"
    std::string region;     // "AS" | "EU" | "NA" | "SA" | "AF" | "OC"
    std::string backend;    // which backend this mirror serves: "arch" | "debian" | "brew" | ...
    double latency_ms = -1; // -1 = not yet measured
    bool   available  = true;
};

// MirrorManager: full pipeline from geo-detection to download URL construction.
//
// Pipeline:
//   1. detect_country()        — IP geolocation (handles VPN, proxies, LANG fallback)
//   2. filter_by_country()     — pick mirrors for detected country/region
//   3. rank_by_latency()       — HEAD request benchmark, sort fastest first
//   4. build_url_list()        — construct ordered URL list for DownloadRequest
//
// The result feeds directly into network::build_mirrored_request().
class MirrorManager {
public:
    explicit MirrorManager(const std::string& config_path);

    // ── Pipeline ──────────────────────────────────────────────────────────────

    // Full auto-select: detect country → filter → benchmark → store result
    void auto_select(const std::string& backend = "");

    // Benchmark all mirrors and sort by latency
    // If backend is specified, only benchmarks mirrors for that backend
    void rank_by_latency(const std::string& backend = "");

    // Manually select a mirror by name
    bool select(const std::string& mirror_name);

    // ── URL construction ──────────────────────────────────────────────────────
    //
    // Given a relative package path (e.g. "/pool/main/nginx_1.25_amd64.deb"),
    // returns an ordered list of full URLs using the ranked mirror list.
    // The first URL is the fastest/preferred mirror; subsequent URLs are fallbacks.
    //
    // Example:
    //   build_url_list("debian", "/pool/main/nginx_1.25_amd64.deb")
    //   → ["https://ftp.jp.debian.org/debian/pool/main/nginx_1.25_amd64.deb",
    //      "https://ftp.de.debian.org/debian/pool/main/nginx_1.25_amd64.deb",
    //      "https://ftp.us.debian.org/debian/pool/main/nginx_1.25_amd64.deb"]
    std::vector<std::string> build_url_list(const std::string& backend,
                                            const std::string& relative_path) const;

    // Convenience: returns the single best URL (no fallbacks)
    std::string best_url(const std::string& backend,
                         const std::string& relative_path) const;

    // ── Query ─────────────────────────────────────────────────────────────────

    std::vector<Mirror> list(const std::string& backend = "") const;
    std::vector<Mirror> list_ranked(const std::string& backend = "") const;  // sorted by latency

    // Returns the currently selected mirror for a backend
    Mirror current(const std::string& backend = "") const;

    // ── Geo-detection ─────────────────────────────────────────────────────────

    // Detect user's country code.
    // Strategy:
    //   1. ipapi.co with CURLOPT_FRESH_CONNECT (bypasses rule-based proxy)
    //   2. ip-country.fly.dev (backup)
    //   3. api.country.is (backup)
    //   4. Parse $LANG / $LC_ALL environment variables
    //   5. Fall back to "US" (globally accessible mirrors)
    std::string detect_country() const;

    // ── Management ────────────────────────────────────────────────────────────

    // Load custom mirrors from config file (user-defined mirror entries)
    void load_config();

    // Save current mirror selection to config
    void save_config() const;

    // Add a custom mirror to the database
    void add_mirror(const Mirror& m);

    // Print mirror status table (for sspm mirror status)
    void print_status() const;

private:
    std::string config_path_;
    std::vector<Mirror> mirrors_;

    // backend → index into mirrors_ of the currently selected mirror
    std::unordered_map<std::string, size_t> selected_;

    std::string fetch_geo_ip() const;
    double ping_mirror(const Mirror& m) const;  // returns ms, -1 on failure
    std::vector<Mirror> mirrors_for(const std::string& backend) const;
};

} // namespace sspm
