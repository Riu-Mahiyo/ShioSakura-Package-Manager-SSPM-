// layer/backends/pip_backend.cpp — pip packages
#include "pip_backend.h"
#include "sspm/log.h"
#include <sstream>

namespace sspm::backends {

std::string PipBackend::pipCmd() {
    if (system("which pip3 > /dev/null 2>&1") == 0) return "pip3";
    if (system("which pip > /dev/null 2>&1") == 0)  return "pip";
    return "";
}

bool PipBackend::is_available() const {
    return !pipCmd().empty();
}

Result PipBackend::install(const Package& pkg) {
    std::string pip = pipCmd();
    if (pip.empty()) {
        return {false, "", "pip 命令未找到"};
    }

    std::string cmd = pip + " install --user " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("安装成功");
        return {true, "安装成功", ""};
    } else {
        // 部分系统 pip 要求 --break-system-packages（Debian 12+）
        SSPM_WARN("尝试 --break-system-packages...");
        cmd += " --break-system-packages";
        SSPM_INFO("执行: " + cmd);
        rc = system(cmd.c_str());
        if (rc == 0) {
            SSPM_INFO("安装成功（--break-system-packages）");
            return {true, "安装成功（--break-system-packages）", ""};
        } else {
            SSPM_ERROR("pip 安装失败 exit=" + std::to_string(rc));
            return {false, "", "pip 安装失败"};
        }
    }
}

Result PipBackend::remove(const Package& pkg) {
    std::string pip = pipCmd();
    if (pip.empty()) {
        return {false, "", "pip 命令未找到"};
    }

    std::string cmd = pip + " uninstall -y " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("卸载成功");
        return {true, "卸载成功", ""};
    } else {
        SSPM_ERROR("pip uninstall 失败 exit=" + std::to_string(rc));
        return {false, "", "pip 卸载失败"};
    }
}

SearchResult PipBackend::search(const std::string& query) {
    std::string pip = pipCmd();
    if (pip.empty()) {
        return {{}, false, "pip 命令未找到"};
    }

    // pip search 在新版本已被禁用，改用 pip index versions 或提示
    SSPM_WARN("pip search 已被禁用，请访问 https://pypi.org/search/?q=" + query);

    // 尝试 pip index versions <pkg>（pip 21.2+）
    std::string cmd = pip + " index versions -- " + query;
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
    if (!output.empty()) {
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            // 跳过空行
            if (line.empty()) {
                continue;
            }
            // 简单解析 pip index versions 输出格式
            if (line.find(query) != std::string::npos) {
                packages.push_back({query, "", line, "pip", ""});
            }
        }
    }

    if (packages.empty()) {
        SSPM_INFO("提示: 请访问 https://pypi.org/search/?q=" + query);
    }

    return {packages, true, ""};
}

Result PipBackend::update() {
    // pip 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result PipBackend::upgrade() {
    std::string pip = pipCmd();
    if (pip.empty()) {
        return {false, "", "pip 命令未找到"};
    }

    SSPM_INFO(pip + " list --outdated  →  " + pip + " install -U");

    // 获取过时包列表
    std::string cmd = pip + " list --outdated --format=freeze";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {false, "", "执行命令失败"};
    }

    std::string output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    if (output.empty()) {
        SSPM_INFO("没有需要更新的 pip 包");
        return {true, "没有需要更新的 pip 包", ""};
    }

    // 解析出包名（格式: name==version）
    std::vector<std::string> toUpdate;
    std::istringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        auto eq = line.find("==");
        if (eq != std::string::npos) {
            toUpdate.push_back(line.substr(0, eq));
        }
    }

    if (toUpdate.empty()) {
        SSPM_INFO("没有需要更新的 pip 包");
        return {true, "没有需要更新的 pip 包", ""};
    }

    SSPM_INFO("更新 " + std::to_string(toUpdate.size()) + " 个包...");
    std::string upgradeCmd = pip + " install --user -U";
    for (const auto& pkg : toUpdate) {
        upgradeCmd += " " + pkg;
    }

    SSPM_INFO("执行: " + upgradeCmd);
    int rc = system(upgradeCmd.c_str());
    if (rc == 0) {
        SSPM_INFO("pip 包更新完成");
        return {true, "pip 包更新完成", ""};
    } else {
        SSPM_ERROR("pip upgrade 失败 exit=" + std::to_string(rc));
        return {false, "", "pip 更新失败"};
    }
}

PackageList PipBackend::list_installed() {
    std::string pip = pipCmd();
    if (pip.empty()) {
        return {};
    }

    std::string cmd = pip + " list --format=freeze";
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
        // 解析格式: name==version
        auto eq = line.find("==");
        if (eq != std::string::npos) {
            std::string name = line.substr(0, eq);
            std::string version = line.substr(eq + 2);
            // 移除换行符
            version.erase(std::remove(version.begin(), version.end(), '\n'), version.end());
            version.erase(std::remove(version.begin(), version.end(), '\r'), version.end());
            packages.push_back({name, version, "", "pip", ""});
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> PipBackend::info(const std::string& name) {
    std::string pip = pipCmd();
    if (pip.empty()) {
        return std::nullopt;
    }

    std::string cmd = pip + " show -- " + name;
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

    // 解析输出，提取包信息
    std::string pkgName = name;
    std::string version, description;
    
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("Version:") == 0) {
            version = line.substr(9);
        } else if (line.find("Summary:") == 0) {
            description = line.substr(9);
        }
    }

    if (!version.empty()) {
        return Package{pkgName, version, description, "pip", ""};
    }

    return std::nullopt;
}

} // namespace sspm::backends
