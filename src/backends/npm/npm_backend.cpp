// layer/backends/npm_backend.cpp — npm global packages
#include "npm_backend.h"
#include "sspm/log.h"

#include <sstream>

namespace sspm::backends {

bool NpmBackend::is_available() const {
    return system("which npm > /dev/null 2>&1") == 0;
}

Result NpmBackend::install(const Package& pkg) {
    std::string cmd = "npm install -g " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("安装成功");
        return {true, "安装成功", ""};
    } else {
        SSPM_ERROR("npm 返回 exit=" + std::to_string(rc));
        return {false, "", "npm 安装失败"};
    }
}

Result NpmBackend::remove(const Package& pkg) {
    std::string cmd = "npm uninstall -g " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("卸载成功");
        return {true, "卸载成功", ""};
    } else {
        SSPM_ERROR("npm uninstall 失败 exit=" + std::to_string(rc));
        return {false, "", "npm 卸载失败"};
    }
}

SearchResult NpmBackend::search(const std::string& query) {
    std::string cmd = "npm search -- " + query;
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
        // 跳过空行和标题行
        if (line.empty() || line.find("NAME") == 0) {
            continue;
        }
        // 简单解析 npm search 输出格式
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t nextSpacePos = line.find(' ', spacePos + 1);
            if (nextSpacePos != std::string::npos) {
                std::string name = line.substr(0, spacePos);
                std::string version = line.substr(spacePos + 1, nextSpacePos - spacePos - 1);
                std::string description = line.substr(nextSpacePos + 1);
                packages.push_back({name, version, description, "npm", ""});
            }
        }
    }

    return {packages, true, ""};
}

Result NpmBackend::update() {
    // npm 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result NpmBackend::upgrade() {
    std::string cmd = "npm update -g";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("npm 全局包更新完成");
        return {true, "npm 全局包更新完成", ""};
    } else {
        SSPM_ERROR("npm update -g 失败 exit=" + std::to_string(rc));
        return {false, "", "npm 更新失败"};
    }
}

PackageList NpmBackend::list_installed() {
    std::string cmd = "npm list -g --depth=0";
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
        // 跳过空行和标题行
        if (line.empty() || line.find("/usr/local/lib/node_modules") == 0) {
            continue;
        }
        // 解析 npm list 输出格式: "├── pkg@1.2.3"
        size_t atPos = line.find('@');
        if (atPos != std::string::npos) {
            size_t nameStart = line.find_first_not_of(" ├──");
            if (nameStart != std::string::npos) {
                std::string name = line.substr(nameStart, atPos - nameStart);
                std::string version = line.substr(atPos + 1);
                // 移除换行符
                version.erase(std::remove(version.begin(), version.end(), '\n'), version.end());
                version.erase(std::remove(version.begin(), version.end(), '\r'), version.end());
                packages.push_back({name, version, "", "npm", ""});
            }
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> NpmBackend::info(const std::string& name) {
    std::string cmd = "npm list -g --depth=0 -- " + name;
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

    // 解析输出: "└── pkg@1.2.3"
    size_t atPos = output.rfind('@');
    if (atPos == std::string::npos) {
        return std::nullopt;
    }
    
    size_t nameStart = output.find_last_of(" └──") + 1;
    if (nameStart == std::string::npos || nameStart >= atPos) {
        return std::nullopt;
    }
    
    std::string pkgName = output.substr(nameStart, atPos - nameStart);
    std::string version = output.substr(atPos + 1);
    // 移除换行符
    version.erase(std::remove(version.begin(), version.end(), '\n'), version.end());
    version.erase(std::remove(version.begin(), version.end(), '\r'), version.end());

    return Package{pkgName, version, "", "npm", ""};
}

} // namespace sspm::backends
