// ═══════════════════════════════════════════════════════════
//  desktop/desktop_integration.cpp — Linux 桌面集成实现
// ═══════════════════════════════════════════════════════════
#include "desktop_integration.h"
#include "../core/exec_engine.h"
#include "../core/logger.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace SSPM {
namespace fs = std::filesystem;

// ── 路径 ─────────────────────────────────────────────────

std::string DesktopIntegration::applicationsDir() {
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base = xdg ? std::string(xdg) :
        std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/.local/share";
    return base + "/applications";
}

std::string DesktopIntegration::mimeappsListPath() {
    const char* xdg = getenv("XDG_CONFIG_HOME");
    std::string base = xdg ? std::string(xdg) :
        std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/.config";
    return base + "/mimeapps.list";
}

bool DesktopIntegration::desktopAvailable() {
    // 检查是否有图形环境
    return getenv("DISPLAY") != nullptr || getenv("WAYLAND_DISPLAY") != nullptr;
}

// ── .desktop 生成 ────────────────────────────────────────

bool DesktopIntegration::installDesktop(const DesktopEntry& entry) {
    // 基本校验
    if (entry.name.empty() || entry.exec.empty()) {
        Logger::error("[desktop] name 和 exec 不能为空");
        return false;
    }

    // 安全校验 exec 路径
    auto execV = InputValidator::safeString(entry.exec, 512);
    if (!execV) {
        Logger::error("[desktop] exec 路径非法: " + execV.reason);
        return false;
    }

    std::string dir = applicationsDir();
    try { fs::create_directories(dir); }
    catch (...) { Logger::error("[desktop] 无法创建目录: " + dir); return false; }

    // 生成安全的文件名（替换空格为 -，小写）
    std::string fileName = entry.name;
    std::replace(fileName.begin(), fileName.end(), ' ', '-');
    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
    // 去掉非法字符
    fileName.erase(std::remove_if(fileName.begin(), fileName.end(),
        [](char c){ return !isalnum(c) && c != '-' && c != '_'; }), fileName.end());

    std::string filePath = dir + "/" + fileName + ".desktop";

    Logger::step("生成 .desktop: " + filePath);

    std::ofstream f(filePath);
    if (!f) {
        Logger::error("[desktop] 无法写入: " + filePath);
        return false;
    }

    f << "[Desktop Entry]\n";
    f << "Version=1.0\n";
    f << "Type=Application\n";
    f << "Name=" << entry.name << "\n";
    f << "Exec=" << entry.exec << "\n";

    if (!entry.icon.empty())       f << "Icon=" << entry.icon << "\n";
    if (!entry.comment.empty())    f << "Comment=" << entry.comment << "\n";
    if (!entry.categories.empty()) f << "Categories=" << entry.categories << "\n";
    if (!entry.mimeTypes.empty())  f << "MimeType=" << entry.mimeTypes << "\n";
    f << "Terminal=" << (entry.terminal ? "true" : "false") << "\n";
    f << "StartupNotify=true\n";

    f.close();

    // chmod 644
    fs::permissions(filePath, fs::perms::owner_read | fs::perms::owner_write |
                               fs::perms::group_read | fs::perms::others_read);

    Logger::ok(".desktop 已生成: " + filePath);
    refreshDesktopDb();
    return true;
}

bool DesktopIntegration::uninstallDesktop(const std::string& appName) {
    std::string dir = applicationsDir();
    std::string fileName = appName;
    std::replace(fileName.begin(), fileName.end(), ' ', '-');
    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
    std::string filePath = dir + "/" + fileName + ".desktop";

    if (!fs::exists(filePath)) {
        Logger::warn("[desktop] 未找到 .desktop 文件: " + filePath);
        return false;
    }

    fs::remove(filePath);
    Logger::ok(".desktop 已删除: " + filePath);
    refreshDesktopDb();
    return true;
}

// 自动检测应用信息
DesktopEntry DesktopIntegration::autoDetect(const std::string& execPath,
                                              const std::string& appName)
{
    DesktopEntry entry;
    entry.name = appName;
    entry.exec = execPath;
    entry.icon = findIcon(appName, execPath);
    entry.categories = "Utility;";
    entry.terminal = false;
    return entry;
}

// 从常见位置查找图标
std::string DesktopIntegration::findIcon(const std::string& pkgName,
                                          const std::string& execPath)
{
    // 按优先级查找图标
    static const char* kIconDirs[] = {
        "/usr/share/icons/hicolor/256x256/apps/",
        "/usr/share/icons/hicolor/128x128/apps/",
        "/usr/share/icons/hicolor/48x48/apps/",
        "/usr/share/pixmaps/",
        nullptr
    };
    static const char* kIconExts[] = { ".png", ".svg", ".xpm", nullptr };

    for (auto dir = kIconDirs; *dir; ++dir) {
        for (auto ext = kIconExts; *ext; ++ext) {
            std::string path = std::string(*dir) + pkgName + *ext;
            if (fs::exists(path)) return path;
        }
    }

    // 尝试从 exec 旁边的 icons/ 目录
    if (!execPath.empty()) {
        fs::path execP(execPath);
        for (auto ext = kIconExts; *ext; ++ext) {
            std::string local = execP.parent_path().string() + "/" + pkgName + *ext;
            if (fs::exists(local)) return local;
        }
    }

    // 没有找到图标，返回包名（桌面系统会尝试主题查找）
    return pkgName;
}

// ── MIME 注册 ────────────────────────────────────────────

bool DesktopIntegration::registerMime(const MimeAssoc& assoc) {
    if (assoc.mimeType.empty() || assoc.appDesktopId.empty()) {
        Logger::error("[mime] mimeType 和 appDesktopId 不能为空");
        return false;
    }

    // 安全校验
    auto mV = InputValidator::safeString(assoc.mimeType, 128);
    auto aV = InputValidator::safeString(assoc.appDesktopId, 128);
    if (!mV || !aV) {
        Logger::error("[mime] 参数非法");
        return false;
    }

    std::string path = mimeappsListPath();
    try { fs::create_directories(fs::path(path).parent_path()); }
    catch (...) {}

    Logger::step("注册 MIME: " + assoc.mimeType + " → " + assoc.appDesktopId);

    // 读取现有文件
    std::vector<std::string> lines;
    std::string section;
    bool inDefaultApps = false;
    bool entryAdded = false;
    {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) lines.push_back(line);
    }

    // 修改或追加 [Default Applications] 段
    std::ofstream f(path);
    if (!f) {
        Logger::error("[mime] 无法写入: " + path);
        return false;
    }

    bool foundSection = false;
    for (auto& line : lines) {
        if (line == "[Default Applications]") {
            foundSection = true;
            inDefaultApps = true;
            f << line << "\n";
            continue;
        }
        if (!line.empty() && line[0] == '[') {
            if (inDefaultApps && !entryAdded) {
                f << assoc.mimeType << "=" << assoc.appDesktopId << "\n";
                entryAdded = true;
            }
            inDefaultApps = false;
        }
        // 跳过已有的相同 mimeType 行
        if (inDefaultApps && line.rfind(assoc.mimeType + "=", 0) == 0) {
            continue;
        }
        f << line << "\n";
    }

    if (!foundSection) {
        f << "\n[Default Applications]\n";
        f << assoc.mimeType << "=" << assoc.appDesktopId << "\n";
    } else if (!entryAdded && inDefaultApps) {
        f << assoc.mimeType << "=" << assoc.appDesktopId << "\n";
    }

    Logger::ok("MIME 关联已注册: " + assoc.mimeType);
    refreshMimeDb();
    return true;
}

bool DesktopIntegration::registerMimeTypes(const std::vector<MimeAssoc>& assocs) {
    bool ok = true;
    for (auto& a : assocs) {
        if (!registerMime(a)) ok = false;
    }
    return ok;
}

bool DesktopIntegration::unregisterMime(const std::string& mimeType,
                                         const std::string& appDesktopId)
{
    std::string path = mimeappsListPath();
    if (!fs::exists(path)) return true;

    std::vector<std::string> lines;
    {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) lines.push_back(line);
    }

    std::ofstream f(path);
    for (auto& line : lines) {
        if (line.rfind(mimeType + "=" + appDesktopId, 0) == 0) continue;
        f << line << "\n";
    }

    Logger::ok("MIME 关联已移除: " + mimeType);
    refreshMimeDb();
    return true;
}

// ── 刷新数据库 ────────────────────────────────────────────

void DesktopIntegration::refreshDesktopDb() {
    if (!ExecEngine::exists("update-desktop-database")) return;

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    ExecEngine::capture("update-desktop-database",
        {applicationsDir()}, opts);
}

void DesktopIntegration::refreshMimeDb() {
    if (!ExecEngine::exists("xdg-mime")) return;

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    ExecEngine::capture("xdg-mime", {"default", ""}, opts);
}

} // namespace SSPM
