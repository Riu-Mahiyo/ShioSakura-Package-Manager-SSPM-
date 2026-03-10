// layer/backends/dpkg_backend.cpp — dpkg 直接操作 .deb 文件
// 设计：
//   sspm install --backend=dpkg ./pkg.deb     → dpkg -i ./pkg.deb
//   sspm remove  --backend=dpkg pkg-name      → dpkg -r pkg-name
//   sky search  --backend=dpkg query         → dpkg -l *query*
#include "dpkg_backend.h"
#include "sspm/log.h"

#include <cstdlib>
#include <sstream>

namespace sspm::backends {

bool DpkgBackend::is_available() const {
    return system("which dpkg > /dev/null 2>&1") == 0;
}

Result DpkgBackend::install(const Package& pkg) {
    // 构建 dpkg -i 命令
    std::string cmd = "dpkg -i " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    // 设置环境变量
    setenv("DEBIAN_FRONTEND", "noninteractive", 1);
    int rc = system(cmd.c_str());
    unsetenv("DEBIAN_FRONTEND");

    if (rc == 0) {
        SSPM_INFO("安装成功");
        return {true, "安装成功", ""};
    } else {
        SSPM_ERROR("dpkg -i 失败 exit=" + std::to_string(rc));
        SSPM_INFO("提示：若有依赖缺失，运行 apt install -f 修复");
        return {false, "", "安装失败"};
    }
}

Result DpkgBackend::remove(const Package& pkg) {
    // 构建 dpkg -r 命令
    std::string cmd = "dpkg -r " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    // 设置环境变量
    setenv("DEBIAN_FRONTEND", "noninteractive", 1);
    int rc = system(cmd.c_str());
    unsetenv("DEBIAN_FRONTEND");

    if (rc == 0) {
        SSPM_INFO("卸载成功");
        return {true, "卸载成功", ""};
    } else {
        SSPM_ERROR("dpkg -r 失败 exit=" + std::to_string(rc));
        return {false, "", "卸载失败"};
    }
}

SearchResult DpkgBackend::search(const std::string& query) {
    // 构建 dpkg -l 命令
    std::string cmd = "dpkg -l *" + query + "*";
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
        // 跳过标题行和空行
        if (line.empty() || line[0] == 'D' || line[0] == 'U' || line[0] == 'i') {
            continue;
        }
        // 简单解析 dpkg -l 输出格式
        std::istringstream line_iss(line);
        std::string status, name, version, description;
        line_iss >> status >> name >> version;
        std::getline(line_iss, description);
        if (!name.empty()) {
            packages.push_back({name, version, description, "dpkg", ""});
        }
    }

    return {packages, true, ""};
}

Result DpkgBackend::update() {
    // dpkg 没有 update 概念，提示用 apt
    SSPM_WARN("dpkg 不支持 update，请使用 apt update");
    return {true, "", ""};
}

Result DpkgBackend::upgrade() {
    // dpkg 没有 upgrade 概念，提示用 apt
    SSPM_WARN("dpkg 不支持全局升级，请使用 apt upgrade");
    return {true, "", ""};
}

PackageList DpkgBackend::list_installed() {
    // 构建 dpkg -l 命令
    std::string cmd = "dpkg -l";
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
        // 跳过标题行和空行
        if (line.empty() || line[0] == 'D' || line[0] == 'U') {
            continue;
        }
        // 简单解析 dpkg -l 输出格式
        std::istringstream line_iss(line);
        std::string status, name, version, description;
        line_iss >> status >> name >> version;
        std::getline(line_iss, description);
        if (!name.empty()) {
            packages.push_back({name, version, description, "dpkg", ""});
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> DpkgBackend::info(const std::string& name) {
    // 构建 dpkg-query 命令
    std::string cmd = "dpkg-query -W -f='${Package}\t${Version}\t${Description}\n' " + name;
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
    std::string pkg_name, version, description;
    if (std::getline(iss, pkg_name, '\t') &&
        std::getline(iss, version, '\t') &&
        std::getline(iss, description)) {
        return Package{pkg_name, version, description, "dpkg", ""};
    }

    return std::nullopt;
}

} // namespace sspm::backends
