// ═══════════════════════════════════════════════════════════
//  core/mirror.cpp — 智能镜像源选择器实现
// ═══════════════════════════════════════════════════════════
#include "mirror.h"
#include "exec_engine.h"
#include "logger.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace SSPM {
namespace fs = std::filesystem;

// ── 内置镜像数据库 ────────────────────────────────────────
// region → backend → mirror_url
const std::map<std::string, std::map<std::string, std::string>>&
MirrorManager::builtinMirrors() {
    static const std::map<std::string, std::map<std::string, std::string>> db = {
        {"CN", {
            {"apt",    "https://mirrors.tuna.tsinghua.edu.cn"},
            {"dnf",    "https://mirrors.tuna.tsinghua.edu.cn/fedora"},
            {"pacman", "https://mirrors.tuna.tsinghua.edu.cn/archlinux/$repo/os/$arch"},
            {"pip",    "https://pypi.tuna.tsinghua.edu.cn/simple"},
            {"npm",    "https://registry.npmmirror.com"},
            {"nix",    "https://mirror.tuna.tsinghua.edu.cn/nix-channels/store"},
            {"cargo",  "https://mirrors.tuna.tsinghua.edu.cn/git/crates.io-index"},
            {"gem",    "https://gems.ruby-china.com"},
        }},
        {"JP", {
            {"apt",    "https://ftp.jaist.ac.jp/pub/Linux/ubuntu"},
            {"pacman", "https://ftp.jaist.ac.jp/pub/Linux/ArchLinux/$repo/os/$arch"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
        {"KR", {
            {"apt",    "https://ftp.harukasan.org/ubuntu"},
            {"pacman", "https://ftp.harukasan.org/archlinux/$repo/os/$arch"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
        {"DE", {
            {"apt",    "https://de.archive.ubuntu.com/ubuntu"},
            {"pacman", "https://mirror.selfnet.de/archlinux/$repo/os/$arch"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
        {"US", {
            {"apt",    "http://us.archive.ubuntu.com/ubuntu"},
            {"pacman", "https://mirror.arizona.edu/archlinux/$repo/os/$arch"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
        {"UK", {
            {"apt",    "http://gb.archive.ubuntu.com/ubuntu"},
            {"pacman", "https://www.mirrorservice.org/sites/ftp.archlinux.org/$repo/os/$arch"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
        {"SG", {
            {"apt",    "https://mirror.sg.gs/ubuntu"},
            {"pacman", "https://mirror.sg.gs/archlinux/$repo/os/$arch"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
        {"AU", {
            {"apt",    "http://au.archive.ubuntu.com/ubuntu"},
            {"pip",    "https://pypi.org/simple"},
            {"npm",    "https://registry.npmjs.org"},
        }},
    };
    return db;
}

// ── 单例 ──────────────────────────────────────────────────
MirrorManager& MirrorManager::instance() {
    static MirrorManager inst;
    return inst;
}

// ── 区域规范化 ────────────────────────────────────────────
std::string MirrorManager::normalizeRegion(const std::string& cc) {
    // ISO 国家代码 → 我们的区域标识
    static const std::map<std::string, std::string> mapping = {
        {"CN", "CN"}, {"TW", "CN"}, {"HK", "CN"}, {"MO", "CN"},  // 大中华区
        {"JP", "JP"}, {"KR", "KR"}, {"SG", "SG"}, {"AU", "AU"},
        {"US", "US"}, {"CA", "US"},
        {"DE", "DE"}, {"FR", "DE"}, {"NL", "DE"}, {"CH", "DE"},
        {"GB", "UK"}, {"IE", "UK"},
        {"IN", "SG"}, {"MY", "SG"}, {"TH", "SG"},
    };
    auto it = mapping.find(cc);
    return it != mapping.end() ? it->second : "US";
}

// ── IP 地理位置查询 ───────────────────────────────────────
std::string MirrorManager::queryIpRegion() {
    // 用 curl 查询 ip-api.com（离线安全，失败不阻塞）
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 5;

    // 尝试 ip-api.com（免费，无需 API key）
    auto r = ExecEngine::capture("curl",
        {"-fsSL", "--connect-timeout", "3",
         "http://ip-api.com/line/?fields=countryCode"}, opts);

    if (r.ok() && r.stdout_out.size() == 2) {
        std::string cc = r.stdout_out.substr(0, 2);
        // 转大写
        std::transform(cc.begin(), cc.end(), cc.begin(), ::toupper);
        Logger::debug("[mirror] IP 查询区域: " + cc);
        return normalizeRegion(cc);
    }

    // 备用：ipinfo.io
    auto r2 = ExecEngine::capture("curl",
        {"-fsSL", "--connect-timeout", "3",
         "https://ipinfo.io/country"}, opts);

    if (r2.ok() && !r2.stdout_out.empty()) {
        std::string cc = r2.stdout_out.substr(0, 2);
        std::transform(cc.begin(), cc.end(), cc.begin(), ::toupper);
        Logger::debug("[mirror] IP 查询区域(ipinfo): " + cc);
        return normalizeRegion(cc);
    }

    return "";  // 查询失败
}

// ── 区域检测主函数 ────────────────────────────────────────
std::string MirrorManager::detectRegion() {
    if (!region_.empty()) return region_;

    // 1. 环境变量优先
    const char* env = getenv("SSPM_REGION");
    if (env && *env) {
        region_ = std::string(env);
        std::transform(region_.begin(), region_.end(), region_.begin(), ::toupper);
        Logger::debug("[mirror] 使用环境变量区域: " + region_);
        return region_;
    }

    // 2. 读取缓存（避免每次启动都查IP）
    const char* home = getenv("HOME");
    std::string cacheFile;
    if (home) cacheFile = std::string(home) + "/.cache/sspm/region";
    if (!cacheFile.empty() && fs::exists(cacheFile)) {
        std::ifstream f(cacheFile);
        std::string cached;
        if (std::getline(f, cached) && !cached.empty() && cached.size() <= 4) {
            region_ = cached;
            return region_;
        }
    }

    // 3. IP 查询
    std::string detected = queryIpRegion();
    if (detected.empty()) detected = "US";  // 默认 US（官方源）
    region_ = detected;

    // 保存缓存
    if (!cacheFile.empty()) {
        try {
            fs::create_directories(fs::path(cacheFile).parent_path());
            std::ofstream f(cacheFile);
            f << region_ << "\n";
        } catch (...) {}
    }

    Logger::info("[mirror] 检测到区域: " + region_);
    return region_;
}

void MirrorManager::setRegion(const std::string& region) {
    region_ = region;
    std::transform(region_.begin(), region_.end(), region_.begin(), ::toupper);
}

// ── 获取最佳镜像 ──────────────────────────────────────────
std::string MirrorManager::getBestMirror(const std::string& backend) {
    std::string region = detectRegion();
    auto& db = builtinMirrors();

    auto rit = db.find(region);
    if (rit != db.end()) {
        auto bit = rit->second.find(backend);
        if (bit != rit->second.end()) {
            return bit->second;
        }
    }

    // 找不到本区域 → 回退 US
    auto usit = db.find("US");
    if (usit != db.end()) {
        auto bit = usit->second.find(backend);
        if (bit != usit->second.end()) return bit->second;
    }

    return "";  // 无镜像配置
}

// ── 应用镜像 ──────────────────────────────────────────────
bool MirrorManager::applyMirror(const std::string& backend) {
    if (isSSPMManaged(backend)) {
        // 已应用，幂等
        return true;
    }

    std::string url = getBestMirror(backend);
    if (url.empty()) {
        Logger::debug("[mirror] 无 " + backend + " 镜像配置");
        return false;
    }

    Logger::step("[mirror] 为 " + backend + " 应用镜像: " + url);

    // pip
    if (backend == "pip") {
        const char* home = getenv("HOME");
        if (!home) return false;
        std::string cfgDir  = std::string(home) + "/.config/pip";
        std::string cfgFile = cfgDir + "/pip.conf";
        try { fs::create_directories(cfgDir); } catch (...) { return false; }

        // 检查是否用户自定义
        if (fs::exists(cfgFile)) {
            std::ifstream f(cfgFile);
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            if (content.find("[sspm-managed]") == std::string::npos &&
                content.find("[global]") != std::string::npos) {
                Logger::debug("[mirror] pip.conf 由用户管理，跳过");
                return false;
            }
        }

        std::ofstream f(cfgFile);
        f << "# SSPM managed - do not edit this section manually\n";
        f << "# [sspm-managed]\n";
        f << "[global]\n";
        f << "index-url = " << url << "\n";
        f << "trusted-host = " << [&]{
            // 提取域名
            std::string u = url;
            auto p = u.find("://");
            if (p != std::string::npos) u = u.substr(p+3);
            auto sl = u.find('/');
            if (sl != std::string::npos) u = u.substr(0, sl);
            return u;
        }() << "\n";
        Logger::ok("[mirror] pip 镜像已设置: " + url);
        return true;
    }

    // npm
    if (backend == "npm") {
        ExecEngine::Options opts;
        opts.captureOutput = false;
        opts.timeoutSec    = 10;
        int rc = ExecEngine::run("npm", {"config", "set", "registry", url}, opts);
        if (rc == 0) Logger::ok("[mirror] npm 镜像已设置: " + url);
        return rc == 0;
    }

    // cargo
    if (backend == "cargo") {
        const char* home = getenv("HOME");
        if (!home) return false;
        std::string cfgDir  = std::string(home) + "/.cargo";
        std::string cfgFile = cfgDir + "/config.toml";
        try { fs::create_directories(cfgDir); } catch (...) { return false; }

        if (fs::exists(cfgFile)) {
            std::ifstream f(cfgFile);
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            if (content.find("[sspm-managed]") == std::string::npos &&
                content.find("[source.crates-io]") != std::string::npos) {
                Logger::debug("[mirror] ~/.cargo/config.toml 由用户管理，跳过");
                return false;
            }
        }

        std::ofstream f(cfgFile, std::ios::app);
        f << "\n# SSPM managed mirror [sspm-managed]\n";
        f << "[source.crates-io]\n";
        f << "replace-with = 'sspm-mirror'\n\n";
        f << "[source.sspm-mirror]\n";
        f << "registry = \"" << url << "\"\n";
        Logger::ok("[mirror] cargo 镜像已设置: " + url);
        return true;
    }

    Logger::debug("[mirror] " + backend + " 镜像应用暂不支持自动配置");
    return false;
}

bool MirrorManager::isSSPMManaged(const std::string& backend) {
    const char* home = getenv("HOME");
    if (!home) return false;

    if (backend == "pip") {
        std::string path = std::string(home) + "/.config/pip/pip.conf";
        if (!fs::exists(path)) return false;
        std::ifstream f(path);
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        return content.find("[sspm-managed]") != std::string::npos;
    }

    return false;
}

} // namespace SSPM
