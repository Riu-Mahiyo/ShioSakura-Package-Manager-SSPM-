// layer/backends/ports_backend.cpp — FreeBSD Ports Collection
#include "ports_backend.h"
#include "sspm/log.h"

#include <filesystem>
#include <sstream>

namespace sspm::backends {
namespace fs = std::filesystem;

bool PortsBackend::is_available() const {
    // 严格限制：只在 FreeBSD 上启用
    // 通过检测 /usr/ports 目录和 FreeBSD 系统标识
#ifdef __FreeBSD__
    return fs::exists("/usr/ports");
#else
    return false;
#endif
}

bool PortsBackend::findPort(const std::string& pkg, std::string& portDir) {
    // 用 whereis 或 /usr/sbin/pkg search 查找 port 路径
    std::string cmd = "whereis -q -s -- " + pkg;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        std::string path;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            path += buffer;
        }
        pclose(pipe);
        
        while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
            path.pop_back();
        if (!path.empty() && fs::exists(path)) {
            portDir = path;
            return true;
        }
    }

    // 搜索 /usr/ports（简单遍历）
    if (fs::exists("/usr/ports")) {
        for (auto& category : fs::directory_iterator("/usr/ports")) {
            std::string candidate = category.path().string() + "/" + pkg;
            if (fs::exists(candidate + "/Makefile")) {
                portDir = candidate;
                return true;
            }
        }
    }

    return false;
}

Result PortsBackend::install(const Package& pkg) {
    std::string portDir;
    if (!findPort(pkg.name, portDir)) {
        SSPM_ERROR("[ports] 找不到 port: " + pkg.name);
        SSPM_INFO("请确认 /usr/ports 已更新: portsnap fetch update");
        return {false, "", "找不到 port"};
    }

    std::string cmd = "make -C " + portDir + " install clean BATCH=yes";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已安装: " + pkg.name);
        return {true, "已安装: " + pkg.name, ""};
    } else {
        SSPM_ERROR("ports install " + pkg.name + " 失败");
        return {false, "", "ports 安装失败"};
    }
}

Result PortsBackend::remove(const Package& pkg) {
    std::string portDir;
    if (!findPort(pkg.name, portDir)) {
        // 回退：使用 pkg 删除
        SSPM_INFO("[ports] 用 pkg 删除 " + pkg.name);
        std::string cmd = "pkg delete -y -- " + pkg.name;
        int rc = system(cmd.c_str());
        if (rc == 0) {
            SSPM_INFO("已卸载: " + pkg.name);
            return {true, "已卸载: " + pkg.name, ""};
        } else {
            SSPM_ERROR("pkg 删除 " + pkg.name + " 失败");
            return {false, "", "pkg 删除失败"};
        }
    }

    std::string cmd = "make -C " + portDir + " deinstall";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已卸载: " + pkg.name);
        return {true, "已卸载: " + pkg.name, ""};
    } else {
        SSPM_ERROR("ports deinstall " + pkg.name + " 失败");
        return {false, "", "ports 卸载失败"};
    }
}

SearchResult PortsBackend::search(const std::string& query) {
    SSPM_INFO("搜索 ports: " + query);

    // 用 pkg 搜索（更快）
    std::string cmd = "pkg search -- " + query;
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
        // 简单解析 pkg search 输出格式
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            std::string name = line.substr(0, spacePos);
            std::string description = line.substr(spacePos + 1);
            packages.push_back({name, "", description, "ports", ""});
        }
    }

    if (packages.empty() && fs::exists("/usr/ports")) {
        // 回退：make search
        SSPM_INFO("也可以使用: make -C /usr/ports search name=" + query);
    }

    return {packages, true, ""};
}

Result PortsBackend::update() {
    // ports 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result PortsBackend::upgrade() {
    std::string cmd = "portsnap fetch update";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc != 0) {
        SSPM_WARN("portsnap 更新失败，尝试 git 方式");
    }

    // 用 portmaster 升级已安装 ports（如果存在）
    if (system("which portmaster > /dev/null 2>&1") == 0) {
        cmd = "portmaster -a --no-confirm";
        SSPM_INFO("执行: " + cmd);
        rc = system(cmd.c_str());
    } else {
        SSPM_INFO("如需升级所有 ports，请安装 portmaster：");
        SSPM_INFO("  pkg install portmaster");
    }

    if (rc == 0) {
        return {true, "Ports 升级完成", ""};
    } else {
        return {false, "", "Ports 升级失败"};
    }
}

PackageList PortsBackend::list_installed() {
    // 使用 pkg 列出已安装的包
    std::string cmd = "pkg info";
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
        // 简单解析 pkg info 输出格式
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t versionPos = line.find(' ', spacePos + 1);
            if (versionPos != std::string::npos) {
                std::string name = line.substr(0, spacePos);
                std::string version = line.substr(spacePos + 1, versionPos - spacePos - 1);
                std::string description = line.substr(versionPos + 1);
                packages.push_back({name, version, description, "ports", ""});
            }
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> PortsBackend::info(const std::string& name) {
    // 使用 pkg 查看包信息
    std::string cmd = "pkg info " + name;
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
    std::string line;
    std::string name_out, version, description;
    
    if (std::getline(iss, line)) {
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t versionPos = line.find(' ', spacePos + 1);
            if (versionPos != std::string::npos) {
                name_out = line.substr(0, spacePos);
                version = line.substr(spacePos + 1, versionPos - spacePos - 1);
                // 读取描述
                if (std::getline(iss, line)) {
                    description = line;
                }
                return Package{name_out, version, description, "ports", ""};
            }
        }
    }

    return std::nullopt;
}

} // namespace sspm::backends
