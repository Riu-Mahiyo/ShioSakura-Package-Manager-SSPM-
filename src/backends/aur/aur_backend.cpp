// layer/backends/aur_backend.cpp — AUR Backend
// 参考 v1.9 aur.sh 的 helper 检测策略
// 安全：AUR 包运行 makepkg，代码可能不可信，未来 Phase 3 加签名校验
#include "aur_backend.h"
#include "sspm/log.h"
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace sspm::backends {
namespace fs = std::filesystem;

std::string AurBackend::aurHelper() {
    // 优先级：yay > paru > makepkg（fallback）
    if (system("which yay > /dev/null 2>&1") == 0)  return "yay";
    if (system("which paru > /dev/null 2>&1") == 0) return "paru";
    return "";  // 无 helper，需手动 makepkg
}

bool AurBackend::is_available() const {
    // AUR 只在 Arch Linux 上有意义
    // 检查 pacman 存在（是 Arch 系的前提），且有 git（makepkg 需要）
    return system("which pacman > /dev/null 2>&1") == 0 && 
           system("which makepkg > /dev/null 2>&1") == 0;
}

Result AurBackend::install(const Package& pkg) {
    // AUR 严禁 root（makepkg 拒绝以 root 运行）
    if (getuid() == 0) {
        SSPM_ERROR("[AUR] 不能以 root 安装 AUR 包（makepkg 安全限制）");
        SSPM_INFO("请切换到普通用户，或使用 sudo -u <user> sspm install --backend=aur <pkg>");
        return {false, "", "AUR 禁止 root"};
    }

    std::string helper = aurHelper();
    if (!helper.empty()) {
        std::string cmd = helper + " -S --noconfirm " + pkg.name;
        SSPM_INFO("执行: " + cmd);

        int rc = system(cmd.c_str());
        if (rc == 0) {
            SSPM_INFO("AUR 安装成功");
            return {true, "AUR 安装成功", ""};
        } else {
            SSPM_ERROR(helper + " 失败 exit=" + std::to_string(rc));
            return {false, "", helper + " 安装失败"};
        }
    }

    // fallback: 使用 makepkg 手动安装
    SSPM_WARN("未找到 AUR helper（yay/paru），使用 makepkg 手动安装");
    return makepkgInstall(pkg.name);
}

Result AurBackend::makepkgInstall(const std::string& pkg) {
    // 需要 git 克隆 AUR 仓库
    if (system("which git > /dev/null 2>&1") != 0) {
        SSPM_ERROR("makepkg 安装需要 git");
        return {false, "", "git not found"};
    }

    const char* home = getenv("HOME");
    std::string buildDir = std::string(home ? home : "/tmp") +
                           "/.cache/sspm/aur/" + pkg;

    SSPM_INFO("git clone https://aur.archlinux.org/" + pkg + ".git " + buildDir);

    // 清理旧目录
    try { fs::remove_all(buildDir); } catch (...) {}

    // 克隆 AUR 仓库
    std::string cloneCmd = "git clone --depth=1 https://aur.archlinux.org/" + pkg + ".git " + buildDir;
    int rc = system(cloneCmd.c_str());

    if (rc != 0) {
        SSPM_ERROR("AUR 仓库克隆失败: " + pkg);
        return {false, "", "AUR 仓库克隆失败"};
    }

    // makepkg -si --noconfirm
    std::string makepkgCmd = "cd " + buildDir + " && makepkg -si --noconfirm";
    SSPM_INFO("执行: " + makepkgCmd);

    rc = system(makepkgCmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已安装: " + pkg);
        return {true, "已安装: " + pkg, ""};
    } else {
        SSPM_ERROR("makepkg 失败 exit=" + std::to_string(rc));
        return {false, "", "makepkg 安装失败"};
    }
}

Result AurBackend::remove(const Package& pkg) {
    // AUR 包通过 pacman 卸载
    std::string cmd = "pacman -R --noconfirm " + pkg.name;
    SSPM_INFO("执行: " + cmd + " (AUR 包通过 pacman 卸载)");

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("卸载成功");
        return {true, "卸载成功", ""};
    } else {
        SSPM_ERROR("pacman 卸载失败 exit=" + std::to_string(rc));
        return {false, "", "pacman 卸载失败"};
    }
}

SearchResult AurBackend::search(const std::string& query) {
    std::string helper = aurHelper();
    if (!helper.empty()) {
        std::string cmd = helper + " -Ss --aur -- " + query;
        SSPM_INFO("执行: " + cmd);

        // 执行命令并捕获输出
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return {{}, false, "执行命令失败"};
        }

        std::string output;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        pclose(pipe);

        // 简单解析输出，提取包信息
        PackageList packages;
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            // 跳过空行
            if (line.empty()) {
                continue;
            }
            // 简单解析 AUR helper 输出格式
            size_t slashPos = line.find('/');
            if (slashPos != std::string::npos) {
                size_t spacePos = line.find(' ', slashPos);
                if (spacePos != std::string::npos) {
                    size_t versionPos = line.find(' ', spacePos + 1);
                    if (versionPos != std::string::npos) {
                        std::string name = line.substr(slashPos + 1, spacePos - slashPos - 1);
                        std::string version = line.substr(spacePos + 1, versionPos - spacePos - 1);
                        std::string description = line.substr(versionPos + 1);
                        packages.push_back({name, version, description, "aur", ""});
                    }
                }
            }
        }

        return {packages, true, ""};
    }

    // 无 helper：用 pacman -Ss 搜索 official + 提示
    SSPM_WARN("AUR 搜索需要 yay/paru，当前只显示官方仓库结果");
    std::string cmd = "pacman -Ss -- " + query;
    SSPM_INFO("执行: " + cmd);

    // 执行命令并捕获输出
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {{}, false, "执行命令失败"};
    }

    std::string output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    // 简单解析输出，提取包信息
    PackageList packages;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        // 简单解析 pacman 输出格式
        size_t slashPos = line.find('/');
        if (slashPos != std::string::npos) {
            size_t spacePos = line.find(' ', slashPos);
            if (spacePos != std::string::npos) {
                size_t versionPos = line.find(' ', spacePos + 1);
                if (versionPos != std::string::npos) {
                    std::string name = line.substr(slashPos + 1, spacePos - slashPos - 1);
                    std::string version = line.substr(spacePos + 1, versionPos - spacePos - 1);
                    std::string description = line.substr(versionPos + 1);
                    packages.push_back({name, version, description, "aur", ""});
                }
            }
        }
    }

    return {packages, true, ""};
}

Result AurBackend::update() {
    // AUR 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result AurBackend::upgrade() {
    std::string helper = aurHelper();
    if (helper.empty()) {
        SSPM_WARN("未找到 AUR helper，无法自动升级 AUR 包");
        SSPM_INFO("请安装 yay: sspm install --backend=aur yay");
        return {true, "", ""};
    }

    std::string cmd = helper + " -Syu --noconfirm";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("AUR 更新完成");
        return {true, "AUR 更新完成", ""};
    } else {
        SSPM_ERROR(helper + " 更新失败 exit=" + std::to_string(rc));
        return {false, "", helper + " 更新失败"};
    }
}

PackageList AurBackend::list_installed() {
    // AUR 包安装后由 pacman 管理，使用 pacman -Q 列出
    std::string cmd = "pacman -Q";
    SSPM_INFO("执行: " + cmd);

    // 执行命令并捕获输出
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {};
    }

    PackageList packages;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        // 简单解析 pacman -Q 输出格式
        std::istringstream line_iss(line);
        std::string name, version, description;
        line_iss >> name >> version;
        // AUR 包没有描述信息，留空
        packages.push_back({name, version, "", "aur", ""});
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> AurBackend::info(const std::string& name) {
    // AUR 包安装后由 pacman 管理，使用 pacman -Q 查看
    std::string cmd = "pacman -Q " + name;
    SSPM_INFO("执行: " + cmd);

    // 执行命令并捕获输出
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }

    std::string output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    // 解析输出
    std::istringstream iss(output);
    std::string pkg_name, version;
    if (iss >> pkg_name >> version) {
        return Package{pkg_name, version, "", "aur", ""};
    }

    return std::nullopt;
}

} // namespace sspm::backends
