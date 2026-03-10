#pragma once
// ═══════════════════════════════════════════════════════════
//  desktop/desktop_integration.h — Linux 桌面集成
//  清单第 4 节：
//    4.1 一键生成 .desktop 文件
//    4.2 一键注册 MIME 关联
//
//  支持所有 Layer 安装的软件（apt/spkg/flatpak/snap/portable）
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>

namespace sspm {

struct DesktopEntry {
    std::string name;           // 应用名称
    std::string exec;           // 可执行文件路径
    std::string icon;           // 图标路径或名称
    std::string comment;        // 注释
    std::string categories;     // 分类，如 "Utility;Network;"
    std::string mimeTypes;      // 关联的 MIME 类型，如 "image/png;image/jpeg;"
    bool        terminal = false; // 是否在终端运行
};

struct MimeAssoc {
    std::string mimeType;       // MIME 类型，如 "image/png"
    std::string appDesktopId;   // .desktop 文件 ID，如 "my-app.desktop"
};

class DesktopIntegration {
public:
    // ── .desktop 文件 ─────────────────────────────────────
    // 生成并安装 .desktop 到 ~/.local/share/applications/
    static bool installDesktop(const DesktopEntry& entry);

    // 卸载 .desktop
    static bool uninstallDesktop(const std::string& appName);

    // 从可执行文件自动检测信息（尽力而为）
    static DesktopEntry autoDetect(const std::string& execPath,
                                    const std::string& appName);

    // ── MIME 注册 ─────────────────────────────────────────
    // 注册 MIME 类型关联（写入 ~/.config/mimeapps.list）
    static bool registerMime(const MimeAssoc& assoc);

    // 批量注册
    static bool registerMimeTypes(const std::vector<MimeAssoc>& assocs);

    // 卸载 MIME 关联
    static bool unregisterMime(const std::string& mimeType,
                                const std::string& appDesktopId);

    // ── 工具 ──────────────────────────────────────────────
    // 刷新桌面数据库（update-desktop-database / xdg-mime）
    static void refreshDesktopDb();
    static void refreshMimeDb();

    // 从包的文件列表中提取图标
    static std::string findIcon(const std::string& pkgName,
                                 const std::string& execPath);

    // 检测桌面环境是否可用
    static bool desktopAvailable();

private:
    static std::string applicationsDir();
    static std::string mimeappsListPath();
};

} // namespace sspm
