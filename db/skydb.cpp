// db/skydb.cpp — SkyDB 完整版（含清单第 11 节所有字段）
#include "skydb.h"
#include "../core/logger.h"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace SSPM {
namespace fs = std::filesystem;

std::map<std::string, PkgRecord> SkyDB::db_;
bool SkyDB::loaded_ = false;

std::string SkyDB::dbPath() {
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base;
    if (xdg && *xdg) base = std::string(xdg) + "/sky";
    else {
        const char* home = getenv("HOME");
        base = std::string(home ? home : "/tmp") + "/.local/share/sky";
    }
    return base + "/installed.json";
}

// 从字符串中提取 JSON key/value 对
static std::map<std::string, std::string>
parseJsonObject(const std::string& s) {
    std::map<std::string, std::string> result;
    size_t pos = 0;
    while (pos < s.size()) {
        auto k1 = s.find('"', pos);
        if (k1 == std::string::npos) break;
        auto k2 = s.find('"', k1 + 1);
        if (k2 == std::string::npos) break;
        std::string key = s.substr(k1 + 1, k2 - k1 - 1);

        auto colon = s.find(':', k2 + 1);
        if (colon == std::string::npos) break;

        // 跳过空白找值
        size_t vstart = colon + 1;
        while (vstart < s.size() && s[vstart] == ' ') vstart++;

        if (vstart >= s.size()) break;

        // 布尔值处理
        if (s[vstart] == 't' || s[vstart] == 'f') {
            bool bval = (s[vstart] == 't');
            result[key] = bval ? "true" : "false";
            pos = vstart + (bval ? 4 : 5);
            continue;
        }

        auto v1 = s.find('"', colon + 1);
        if (v1 == std::string::npos) break;
        auto v2 = s.find('"', v1 + 1);
        if (v2 == std::string::npos) break;
        result[key] = s.substr(v1 + 1, v2 - v1 - 1);
        pos = v2 + 1;
    }
    return result;
}

// 当前平台字符串
static std::string currentPlatform() {
    struct utsname {
        char sysname[256];
        char machine[256];
    };
    // 简单判断
#if defined(__linux__)
    const char* sys = "linux";
#elif defined(__APPLE__)
    const char* sys = "macos";
#elif defined(__FreeBSD__)
    const char* sys = "freebsd";
#else
    const char* sys = "unknown";
#endif

#if defined(__x86_64__)
    const char* arch = "x86_64";
#elif defined(__aarch64__)
    const char* arch = "aarch64";
#else
    const char* arch = "unknown";
#endif

    return std::string(sys) + "/" + arch;
}

void SkyDB::load() {
    if (loaded_) return;
    loaded_ = true;

    std::ifstream f(dbPath());
    if (!f.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

    size_t pos = 0;
    while (pos < content.size()) {
        auto k1 = content.find('"', pos);
        if (k1 == std::string::npos) break;
        auto k2 = content.find('"', k1 + 1);
        if (k2 == std::string::npos) break;
        std::string pkg = content.substr(k1 + 1, k2 - k1 - 1);

        auto brace1 = content.find('{', k2 + 1);
        if (brace1 == std::string::npos) break;
        auto brace2 = content.find('}', brace1 + 1);
        if (brace2 == std::string::npos) break;

        std::string block = content.substr(brace1 + 1, brace2 - brace1 - 1);
        auto fields = parseJsonObject(block);

        PkgRecord rec;
        rec.name              = pkg;
        rec.backend           = fields.count("backend")  ? fields["backend"]  : "";
        rec.version           = fields.count("version")  ? fields["version"]  : "";
        rec.platform          = fields.count("platform") ? fields["platform"] : "";
        rec.hash              = fields.count("hash")     ? fields["hash"]     : "";
        rec.desktopRegistered = fields.count("desktop_registered") &&
                                fields["desktop_registered"] == "true";
        rec.mimeRegistered    = fields.count("mime_registered") &&
                                fields["mime_registered"] == "true";

        if (!rec.name.empty() && !rec.backend.empty())
            db_[pkg] = rec;

        pos = brace2 + 1;
    }
}

void SkyDB::save() {
    std::string path = dbPath();
    try { fs::create_directories(fs::path(path).parent_path()); }
    catch (...) { Logger::error("SkyDB: 无法创建目录"); return; }

    std::ofstream f(path);
    if (!f) { Logger::error("SkyDB: 无法写入 " + path); return; }

    auto boolStr = [](bool b) { return b ? "true" : "false"; };

    f << "{\n";
    bool first = true;
    for (auto& [name, rec] : db_) {
        if (!first) f << ",\n";
        first = false;

        char ts[32] = "unknown";
        if (rec.installTime) {
            std::tm* tm = std::localtime(&rec.installTime);
            std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", tm);
        }

        f << "  \"" << name << "\": {\n"
          << "    \"backend\": \""             << rec.backend           << "\",\n"
          << "    \"version\": \""             << rec.version           << "\",\n"
          << "    \"platform\": \""            << rec.platform          << "\",\n"
          << "    \"hash\": \""                << rec.hash              << "\",\n"
          << "    \"desktop_registered\": "    << boolStr(rec.desktopRegistered) << ",\n"
          << "    \"mime_registered\": "       << boolStr(rec.mimeRegistered)    << ",\n"
          << "    \"install_time\": \""        << ts                    << "\"\n"
          << "  }";
    }
    f << "\n}\n";
}

void SkyDB::recordInstall(const std::string& pkg,
                           const std::string& backend,
                           const std::string& version,
                           const std::string& platform,
                           const std::string& hash)
{
    if (!loaded_) load();
    PkgRecord rec;
    rec.name        = pkg;
    rec.backend     = backend;
    rec.version     = version;
    rec.platform    = platform.empty() ? currentPlatform() : platform;
    rec.hash        = hash;
    rec.installTime = std::time(nullptr);
    db_[pkg] = rec;
    save();
}

void SkyDB::updateDesktopStatus(const std::string& pkg,
                                 bool desktopReg, bool mimeReg)
{
    if (!loaded_) load();
    auto it = db_.find(pkg);
    if (it == db_.end()) return;
    it->second.desktopRegistered = desktopReg;
    it->second.mimeRegistered    = mimeReg;
    save();
}

void SkyDB::recordRemove(const std::string& pkg) {
    if (!loaded_) load();
    db_.erase(pkg);
    save();
}

bool SkyDB::isInstalled(const std::string& pkg) {
    if (!loaded_) load();
    return db_.count(pkg) > 0;
}

const PkgRecord* SkyDB::query(const std::string& pkg) {
    if (!loaded_) load();
    auto it = db_.find(pkg);
    return (it != db_.end()) ? &it->second : nullptr;
}

std::map<std::string, PkgRecord> SkyDB::all() {
    if (!loaded_) load();
    return db_;
}

} // namespace SSPM
