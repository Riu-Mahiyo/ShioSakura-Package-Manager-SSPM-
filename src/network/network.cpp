#include "sspm/network.h"
#include "sspm/mirror.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <future>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;
namespace sspm::network {

static size_t write_to_file(void* ptr, size_t size, size_t nmemb, FILE* fp) {
    return fwrite(ptr, size, nmemb, fp);
}

static size_t write_to_string(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

static int progress_cb(void* data, curl_off_t total, curl_off_t downloaded, curl_off_t, curl_off_t) {
    auto* cb = reinterpret_cast<ProgressCallback*>(data);
    if (cb && *cb) (*cb)(static_cast<double>(downloaded), static_cast<double>(total));

    // Default progress bar if no specific callback provided
    if (total > 0) {
        int width = 40;
        double pct = static_cast<double>(downloaded) / static_cast<double>(total);
        int pos = static_cast<int>(width * pct);
        std::cout << "\r[";
        for (int i = 0; i < width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(pct * 100.0) << "% " << std::flush;
        if (downloaded == total) std::cout << "\n";
    }
    return 0;
}

// ── Single download with resume + mirror fallback ────────────────────────────
DownloadResult download(const DownloadRequest& req, ProgressCallback on_progress) {
    std::vector<std::string> urls = { req.url };
    urls.insert(urls.end(), req.fallback_urls.begin(), req.fallback_urls.end());

    for (auto& url : urls) {
        CURL* curl = curl_easy_init();
        if (!curl) continue;

        // Resume: check existing partial file
        curl_off_t resume_from = 0;
        if (req.resume && fs::exists(req.dest_path)) {
            std::error_code ec;
            resume_from = static_cast<curl_off_t>(fs::file_size(req.dest_path, ec));
            if (!ec) {
                std::cout << "[net] Resuming at " << resume_from << " bytes: " << url << "\n";
            } else {
                resume_from = 0;
            }
        }

        FILE* fp = fopen(req.dest_path.c_str(), resume_from ? "ab" : "wb");
        if (!fp) {
            curl_easy_cleanup(curl);
            return { false, "", "Cannot open dest: " + req.dest_path };
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, resume_from);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(req.timeout_sec));
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

        if (on_progress) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &on_progress);
        }

        CURLcode res = curl_easy_perform(curl);
        curl_off_t speed = 0;
        curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &speed);
        fclose(fp);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            std::cout << "[net] Downloaded: " << req.dest_path
                      << " (" << static_cast<double>(speed) / 1024.0 << " KB/s)\n";
            return { true, req.dest_path, "", static_cast<double>(speed) };
        }

        // Cleanup on failure if not resuming
        if (!req.resume) {
            std::error_code ec;
            fs::remove(req.dest_path, ec);
        }

        std::cerr << "[net] Failed (" << url << "): " << curl_easy_strerror(res)
                  << " — trying fallback\n";
    }
    return { false, "", "All mirrors failed for: " + req.url };
}

// ── Parallel download with thread pool ───────────────────────────────────────
std::vector<DownloadResult> download_parallel(
    const std::vector<DownloadRequest>& requests,
    int max_parallel,
    ProgressCallback on_progress)
{
    std::vector<DownloadResult> results(requests.size());
    std::vector<std::future<DownloadResult>> futures;
    futures.reserve(requests.size());

    // Semaphore-like: submit in batches of max_parallel
    size_t i = 0;
    while (i < requests.size()) {
        futures.clear();
        size_t batch_end = std::min(i + (size_t)max_parallel, requests.size());
        for (size_t j = i; j < batch_end; j++) {
            futures.push_back(std::async(std::launch::async,
                [&requests, j, &on_progress]() {
                    return download(requests[j], on_progress);
                }));
        }
        for (size_t j = 0; j < futures.size(); j++)
            results[i + j] = futures[j].get();
        i = batch_end;
    }
    return results;
}

// ── Fetch text (repo JSON, metadata) ────────────────────────────────────────
std::string fetch_text(const std::string& url, int timeout_sec) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";
    std::string out;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_sec);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return out;
}

bool sync_metadata(const std::string& url, const std::string& dest_path) {
    std::string data = fetch_text(url, 15);
    if (data.empty()) return false;
    std::ofstream f(dest_path);
    f << data;
    return true;
}

} // namespace sspm::network

// ── Mirror-aware request builder ─────────────────────────────────────────────
// Queries MirrorManager for the fastest mirror list, then builds a DownloadRequest
// with the best URL first and all fallback mirrors populated.
// This is the recommended way to download package files in the install pipeline.

#include "sspm/mirror.h"

namespace sspm::network {

DownloadRequest build_mirrored_request(const std::string& relative_path,
                                       const std::string& dest_path,
                                       const MirrorManager& mirror_mgr) {
    // Infer backend from path (simple heuristics)
    std::string backend = "sspm";
    if (relative_path.find("/debian/")  != std::string::npos) backend = "debian";
    else if (relative_path.find("/archlinux/") != std::string::npos) backend = "arch";
    else if (relative_path.find("/ubuntu/")    != std::string::npos) backend = "ubuntu";
    else if (relative_path.find("/fedora/")    != std::string::npos) backend = "fedora";
    else if (relative_path.find("/alpine/")    != std::string::npos) backend = "alpine";
    else if (relative_path.find("/homebrew/")  != std::string::npos) backend = "brew";

    auto urls = mirror_mgr.build_url_list(backend, relative_path);

    DownloadRequest req;
    req.dest_path = dest_path;
    req.resume    = true;

    if (!urls.empty()) {
        req.url = urls[0];
        req.fallback_urls = std::vector<std::string>(urls.begin() + 1, urls.end());
    } else {
        // No mirrors configured — use the path as a direct URL (fallback)
        req.url = relative_path;
    }

    return req;
}

// Expose mirror_used field tracking in download()
// (Patch: record which URL actually succeeded in DownloadResult)

} // namespace sspm::network
