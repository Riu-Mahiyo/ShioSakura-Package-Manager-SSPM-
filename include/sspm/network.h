#pragma once
#include <string>
#include <vector>
#include <functional>

namespace sspm::network {

struct DownloadRequest {
    std::string url;
    std::string dest_path;
    std::vector<std::string> fallback_urls; // populated by MirrorManager::build_url_list()
    bool resume = true;                     // resume partial downloads
    int  timeout_sec = 300;
};

struct DownloadResult {
    bool        success;
    std::string path;
    std::string error;
    double      speed_bps = 0;
    std::string mirror_used;  // which mirror actually served the file
};

using ProgressCallback = std::function<void(double downloaded, double total)>;

// ── Core API ──────────────────────────────────────────────────────────────────

// Download a single file (with resume + mirror fallback in order of fallback_urls)
DownloadResult download(const DownloadRequest& req,
                        ProgressCallback on_progress = nullptr);

// Download multiple files in parallel (thread pool, max_parallel workers)
std::vector<DownloadResult> download_parallel(
    const std::vector<DownloadRequest>& requests,
    int max_parallel = 4,
    ProgressCallback on_progress = nullptr);

// Fetch text content (repo JSON, metadata, registry)
std::string fetch_text(const std::string& url, int timeout_sec = 10);

// Sync repo metadata file
bool sync_metadata(const std::string& url, const std::string& dest_path);

// ── Mirror-aware download ─────────────────────────────────────────────────────
// Higher-level function: given a relative path (e.g. "/pool/main/nginx.deb"),
// the MirrorManager is queried for an ordered mirror list, then download() is
// called with the resulting URL list (fastest mirror first, fallbacks after).
//
// Usage:
//   auto req = network::build_mirrored_request(
//       "/pool/main/nginx_1.25.3_amd64.deb",
//       dest_path,
//       mirror_mgr
//   );
//   auto result = network::download(req);

namespace sspm {
class MirrorManager;  // forward declare — defined in mirror.h
} // namespace sspm

DownloadRequest build_mirrored_request(const std::string& relative_path,
                                       const std::string& dest_path,
                                       const sspm::MirrorManager& mirror_mgr);

} // namespace sspm::network
