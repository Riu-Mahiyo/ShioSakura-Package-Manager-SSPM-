#pragma once
// ═══════════════════════════════════════════════════════════
//  db/skydb.h — SkyDB（完整版）
//
//  清单第 11 节要求的完整字段：
//    name, version, layer, platform,
//    desktop_registered, mime_registered,
//    install_time, hash
//
//  存储：~/.local/share/sspm/installed.json
// ═══════════════════════════════════════════════════════════
#include <string>
#include <map>
#include <ctime>

namespace SSPM {

struct PkgRecord {
    std::string name;
    std::string version;
    std::string backend;            // "apt", "pacman", "flatpak", ...
    std::string platform;           // "linux/x86_64", "macos/arm64", ...
    bool        desktopRegistered = false;  // .desktop 已生成
    bool        mimeRegistered    = false;  // MIME 关联已注册
    std::string hash;               // SHA256 of package (optional)
    std::time_t installTime = 0;
};

class SkyDB {
public:
    static void load();
    static void save();

    // 记录安装
    static void recordInstall(const std::string& pkg,
                               const std::string& backend,
                               const std::string& version  = "",
                               const std::string& platform = "",
                               const std::string& hash     = "");

    // 更新桌面集成状态
    static void updateDesktopStatus(const std::string& pkg,
                                     bool desktopReg,
                                     bool mimeReg);

    static void recordRemove(const std::string& pkg);

    static bool              isInstalled(const std::string& pkg);
    static const PkgRecord*  query(const std::string& pkg);
    static std::map<std::string, PkgRecord> all();
    static std::string dbPath();

private:
    static std::map<std::string, PkgRecord> db_;
    static bool loaded_;
};

} // namespace SSPM
