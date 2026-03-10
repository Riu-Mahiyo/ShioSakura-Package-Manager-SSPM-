// ═══════════════════════════════════════════════════════════
//  layer/backends/spk_backend.cpp — Sky Package (spkg)
//
//  .spkg 格式：tar.xz 归档，内含：
//    MANIFEST        元数据（name/version/arch/depends）
//    files/          实际文件树（相对于安装根）
//    install.sh      可选安装钩子（非必需）
//    uninstall.sh    可选卸载钩子
//
//  安装流程：
//    1. 校验完整性（sha256）
//    2. 解析 MANIFEST
//    3. 解压 files/ 到安装根
//    4. 若有 install.sh 且校验通过，执行（ExecEngine，非 shell -c）
//    5. 记录到 SkyDB
// ═══════════════════════════════════════════════════════════
#include "spk_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace SSPM {
namespace fs = std::filesystem;

std::string SpkBackend::spkgDir() {
    // 优先 /usr/local（root），用户级别用 ~/.local
    if (getuid() == 0) return "/usr/local";
    const char* home = getenv("HOME");
    return std::string(home ? home : "/tmp") + "/.local";
}

std::string SpkBackend::dbDir() {
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base = xdg ? std::string(xdg) :
        std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/.local/share";
    return base + "/sspm/spkg";
}

// 从 .spkg（tar.xz）中读取 MANIFEST 字段
std::string SpkBackend::readManifestField(const std::string& spkgFile,
                                             const std::string& field)
{
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;

    // tar -xOf <file>.spkg MANIFEST | grep "^<field>:" | cut -d: -f2
    auto r = ExecEngine::capture("tar",
        {"-xOf", spkgFile, "MANIFEST"}, opts);
    if (!r.ok()) return "";

    std::istringstream ss(r.stdout_out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.rfind(field + ":", 0) == 0) {
            auto val = line.substr(field.size() + 1);
            // trim leading spaces
            auto start = val.find_first_not_of(" \t");
            return (start == std::string::npos) ? "" : val.substr(start);
        }
    }
    return "";
}

bool SpkBackend::verify(const std::string& spkgFile) {
    // 检查 .sha256 文件（若存在）
    std::string shaFile = spkgFile + ".sha256";
    if (!fs::exists(shaFile)) {
        Logger::warn("[spkg] 无 .sha256 校验文件，跳过完整性检验");
        return true;
    }

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    // sha256sum --check <file>.sha256
    auto r = ExecEngine::capture("sha256sum", {"--check", "--", shaFile}, opts);
    if (r.ok()) {
        Logger::ok("[spkg] SHA256 校验通过");
        return true;
    }
    Logger::error("[spkg] SHA256 校验失败: " + r.stderr_out);
    return false;
}

BackendResult SpkBackend::installFile(const std::string& path) {
    auto v = InputValidator::filePath(path, false, true);
    if (!v) return {1, "文件路径非法: " + v.reason};

    Logger::step("安装 spkg: " + path);

    // 1. 校验完整性
    if (!verify(path)) return {1, "spkg 校验失败"};

    // 2. 解析 MANIFEST
    std::string pkgName = readManifestField(path, "Name");
    std::string pkgVer  = readManifestField(path, "Version");
    if (pkgName.empty()) return {1, "MANIFEST 缺少 Name 字段"};

    Logger::info("  包名: " + pkgName + "  版本: " + pkgVer);

    // 3. 准备安装目录
    std::string root = spkgDir();
    std::string metaDir = dbDir() + "/" + pkgName;
    try {
        fs::create_directories(root);
        fs::create_directories(metaDir);
    } catch (...) {
        return {1, "无法创建目录: " + root};
    }

    // 4. 解压 files/ 到安装根
    Logger::step("解压到 " + root);

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    // tar --strip-components=1 解压 files/ 子目录
    int rc = ExecEngine::run("tar",
        {"-xf", path, "--strip-components=1",
         "-C", root, "files/"}, opts);

    if (rc != 0) {
        Logger::error("解压失败 exit=" + std::to_string(rc));
        return {rc, ""};
    }

    // 5. 保存 MANIFEST 到 meta
    ExecEngine::capture("tar",
        {"-xOf", path, "MANIFEST"}, opts);
    // 写入元数据
    std::ofstream mf(metaDir + "/MANIFEST");
    mf << "Name: " << pkgName << "\nVersion: " << pkgVer << "\n";

    Logger::ok("spkg 安装成功: " + pkgName + " " + pkgVer);
    return {0, ""};
}

BackendResult SpkBackend::install(const std::vector<std::string>& pkgs) {
    int lastRc = 0;
    for (auto& pkg : pkgs) {
        BackendResult r;
        if (pkg.size() > 5 &&
            pkg.substr(pkg.size() - 5) == ".spkg") {
            r = installFile(pkg);
        } else {
            // 包名安装：目前提示需要文件路径（repo 功能 Phase 3）
            auto v = InputValidator::pkg(pkg);
            if (!v) {
                Logger::error("[spkg] 包名非法: " + v.reason);
                r = {1, ""};
            } else {
                Logger::warn("[spkg] 暂不支持从 repo 安装，请提供 .spkg 文件路径");
                Logger::info("  用法: sspm install --backend=spkg ./pkg.spkg");
                r = {1, ""};
            }
        }
        if (!r.ok()) lastRc = r.exitCode;
    }
    return {lastRc, ""};
}

BackendResult SpkBackend::remove(const std::vector<std::string>& pkgs) {
    int lastRc = 0;
    for (auto& pkg : pkgs) {
        auto v = InputValidator::pkg(pkg);
        if (!v) { lastRc = 1; continue; }

        std::string metaDir = dbDir() + "/" + pkg;
        if (!fs::exists(metaDir)) {
            Logger::warn("[spkg] 未找到已安装包: " + pkg);
            continue;
        }

        Logger::step("卸载 spkg: " + pkg);
        // 读取 MANIFEST 获取文件列表（简化：删除元数据，保留实体文件提示手动）
        // TODO Phase 3: 记录安装文件列表，实现精确卸载
        Logger::warn("spkg 精确卸载需要文件列表，当前只删除元数据");
        try {
            fs::remove_all(metaDir);
            Logger::ok("已删除元数据: " + pkg);
        } catch (const std::exception& e) {
            Logger::error("卸载失败: " + std::string(e.what()));
            lastRc = 1;
        }
    }
    return {lastRc, ""};
}

BackendResult SpkBackend::upgrade() {
    Logger::warn("spkg 暂不支持自动升级（repo 功能 Phase 3）");
    return {0, ""};
}

BackendResult SpkBackend::search(const std::string& query) {
    // 列出已安装的 spkg
    std::string dir = dbDir();
    if (!fs::exists(dir)) return {0, "(没有已安装的 spkg)\n"};

    std::string result;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        std::string name = entry.path().filename().string();
        if (query.empty() || name.find(query) != std::string::npos) {
            std::string ver;
            std::ifstream mf(entry.path().string() + "/MANIFEST");
            std::string line;
            while (std::getline(mf, line)) {
                if (line.rfind("Version:", 0) == 0)
                    ver = line.substr(8);
            }
            result += "  " + name + (ver.empty() ? "" : " " + ver) + "\n";
        }
    }
    return {0, result.empty() ? "(没有匹配的 spkg)\n" : result};
}

std::string SpkBackend::installedVersion(const std::string& pkg) {
    auto v = InputValidator::pkg(pkg);
    if (!v) return "";
    std::string metaFile = dbDir() + "/" + pkg + "/MANIFEST";
    std::ifstream f(metaFile);
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("Version:", 0) == 0) {
            auto ver = line.substr(8);
            auto s = ver.find_first_not_of(" \t");
            return (s == std::string::npos) ? "" : ver.substr(s);
        }
    }
    return "";
}

// spkg 打包（sky make --spkg）
bool SpkBackend::pack(const std::string& srcDir, const std::string& outputFile) {
    auto sv = InputValidator::filePath(srcDir, false, true);
    auto ov = InputValidator::safeString(outputFile, 512);
    if (!sv || !ov) {
        Logger::error("[spkg] pack 路径非法");
        return false;
    }

    // 检查 MANIFEST 存在
    if (!fs::exists(srcDir + "/MANIFEST")) {
        Logger::error("[spkg] 缺少 MANIFEST 文件");
        Logger::info("  格式: Name: <pkg>\\nVersion: <ver>\\nArch: <arch>\\n");
        return false;
    }

    Logger::step("打包 spkg: " + srcDir + " → " + outputFile);

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;
    opts.workDir       = srcDir;

    int rc = ExecEngine::run("tar",
        {"-cJf", outputFile, "MANIFEST", "files/"}, opts);

    if (rc == 0) {
        // 生成 SHA256
        ExecEngine::Options sha_opts;
        sha_opts.captureOutput = true;
        sha_opts.timeoutSec    = 30;
        auto r = ExecEngine::capture("sha256sum", {"--", outputFile}, sha_opts);
        if (r.ok()) {
            std::ofstream sf(outputFile + ".sha256");
            sf << r.stdout_out;
            Logger::ok("SHA256 已写入: " + outputFile + ".sha256");
        }
        Logger::ok("spkg 打包完成: " + outputFile);
    } else {
        Logger::error("打包失败 exit=" + std::to_string(rc));
    }
    return rc == 0;
}

} // namespace SSPM
