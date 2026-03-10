// layer/backends/portable_backend.cpp — AppImage / Portable 包管理
#include "portable_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <sys/stat.h>

namespace SSPM {
namespace fs = std::filesystem;

std::string PortableBackend::portableDir() {
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base = xdg ? std::string(xdg) : (
        std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/.local/share"
    );
    return base + "/sspm/portable";
}

std::string PortableBackend::binDir() {
    const char* home = getenv("HOME");
    return std::string(home ? home : "/tmp") + "/.local/bin";
}

bool PortableBackend::available() const {
    // AppImage 运行需要 FUSE（Linux kernel module）
    // 简单检测：/dev/fuse 存在
    return access("/dev/fuse", F_OK) == 0;
}

BackendResult PortableBackend::installFromUrl(const std::string& url,
                                               const std::string& appName)
{
    // 安全校验 URL
    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
        return {1, "只支持 http/https URL"};
    }

    std::string destDir = portableDir() + "/" + appName;
    std::string destFile = destDir + "/" + appName + ".AppImage";

    try { fs::create_directories(destDir); }
    catch (...) { return {1, "无法创建目录: " + destDir}; }

    Logger::step("下载 " + url + " → " + destFile);

    auto r = ExecEngine::runCurl(url, 120, true);
    if (!r.ok()) {
        Logger::error("下载失败: " + r.diagnose());
        return {r.exitCode, r.stderr_out};
    }

    // 写入文件
    {
        std::ofstream f(destFile, std::ios::binary);
        if (!f) return {1, "无法写入文件: " + destFile};
        f.write(r.stdout_out.data(), (std::streamsize)r.stdout_out.size());
    }

    // chmod +x
    ::chmod(destFile.c_str(), 0755);

    // 创建符号链接到 ~/.local/bin/<appName>
    try {
        fs::create_directories(binDir());
        std::string linkPath = binDir() + "/" + appName;
        if (fs::exists(linkPath)) fs::remove(linkPath);
        fs::create_symlink(destFile, linkPath);
        Logger::ok("符号链接已创建: " + linkPath);
    } catch (const std::exception& e) {
        Logger::warn("符号链接创建失败: " + std::string(e.what()));
    }

    Logger::ok("AppImage 安装完成: " + destFile);
    return {0, ""};
}

BackendResult PortableBackend::installFromFile(const std::string& path,
                                                const std::string& appName)
{
    auto v = InputValidator::filePath(path, false, true);
    if (!v) return {1, "文件路径非法: " + v.reason};

    std::string destDir  = portableDir() + "/" + appName;
    std::string destFile = destDir + "/" + appName + ".AppImage";

    try { fs::create_directories(destDir); }
    catch (...) { return {1, "无法创建目录: " + destDir}; }

    Logger::step("复制 " + path + " → " + destFile);

    try {
        fs::copy_file(path, destFile, fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        return {1, "复制失败: " + std::string(e.what())};
    }

    ::chmod(destFile.c_str(), 0755);

    try {
        fs::create_directories(binDir());
        std::string linkPath = binDir() + "/" + appName;
        if (fs::exists(linkPath)) fs::remove(linkPath);
        fs::create_symlink(destFile, linkPath);
        Logger::ok("符号链接已创建: " + linkPath);
    } catch (...) {}

    Logger::ok("AppImage 安装完成: " + destFile);
    return {0, ""};
}

BackendResult PortableBackend::install(const std::vector<std::string>& pkgs) {
    int lastRc = 0;
    for (auto& pkg : pkgs) {
        BackendResult r;
        // 判断是 URL 还是本地路径还是包名
        if (pkg.rfind("http://", 0) == 0 || pkg.rfind("https://", 0) == 0) {
            // URL：取文件名作为包名
            std::string appName = pkg.substr(pkg.rfind('/') + 1);
            // 去掉 .AppImage 扩展名
            auto ext = appName.rfind(".AppImage");
            if (ext != std::string::npos) appName = appName.substr(0, ext);
            auto nameV = InputValidator::safeString(appName, 64);
            if (!nameV) appName = "app";
            r = installFromUrl(pkg, appName);
        } else if (pkg[0] == '/' || pkg.rfind("./", 0) == 0) {
            // 本地文件
            std::string appName = fs::path(pkg).stem().string();
            auto nameV = InputValidator::safeString(appName, 64);
            if (!nameV) appName = "app";
            r = installFromFile(pkg, appName);
        } else {
            Logger::error("[portable] 不支持包名安装（尚未实现注册表），请提供 URL 或文件路径");
            r = {1, "需要 URL 或文件路径"};
        }
        if (!r.ok()) lastRc = r.exitCode;
    }
    return {lastRc, ""};
}

BackendResult PortableBackend::remove(const std::vector<std::string>& pkgs) {
    int lastRc = 0;
    for (auto& pkg : pkgs) {
        auto v = InputValidator::pkg(pkg);
        if (!v) { lastRc = 1; continue; }

        std::string destDir  = portableDir() + "/" + pkg;
        std::string linkPath = binDir() + "/" + pkg;

        Logger::step("删除 portable 包: " + pkg);
        try {
            if (fs::exists(linkPath)) fs::remove(linkPath);
            if (fs::exists(destDir))  fs::remove_all(destDir);
            Logger::ok("已删除: " + pkg);
        } catch (const std::exception& e) {
            Logger::error("删除失败: " + std::string(e.what()));
            lastRc = 1;
        }
    }
    return {lastRc, ""};
}

BackendResult PortableBackend::upgrade() {
    Logger::warn("portable 包不支持自动升级（需重新安装新版本）");
    return {0, ""};
}

BackendResult PortableBackend::search(const std::string& query) {
    // 列出本地已安装的 portable 包
    std::string dir = portableDir();
    if (!fs::exists(dir)) {
        return {0, "(没有已安装的 portable 包)\n"};
    }

    std::string result;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        std::string name = entry.path().filename().string();
        if (query.empty() || name.find(query) != std::string::npos) {
            result += "  " + name + "\n";
        }
    }
    return {0, result.empty() ? "(没有匹配的 portable 包)\n" : result};
}

std::string PortableBackend::installedVersion(const std::string& pkg) {
    std::string appImage = portableDir() + "/" + pkg + "/" + pkg + ".AppImage";
    if (!fs::exists(appImage)) return "";
    // AppImage 没有标准版本查询方式，返回文件修改时间
    auto t = fs::last_write_time(appImage);
    return "installed";
}

} // namespace SSPM
