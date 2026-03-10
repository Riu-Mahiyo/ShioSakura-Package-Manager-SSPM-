// layer/backends/gem_backend.cpp — Ruby gem 包管理器
#include "gem_backend.h"
#include "sspm/log.h"

#include <sstream>
#include <unistd.h>

namespace sspm::backends {

bool GemBackend::is_available() const {
    return system("which gem > /dev/null 2>&1") == 0;
}

Result GemBackend::install(const Package& pkg) {
    // 非 root 使用 --user-install
    bool isRoot = (getuid() == 0);
    std::string cmd = "gem install";
    if (!isRoot) {
        cmd += " --user-install";
    }
    cmd += " -- " + pkg.name;

    SSPM_INFO("执行: " + cmd);
    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已安装: " + pkg.name);
        return {true, "已安装: " + pkg.name, ""};
    } else {
        SSPM_ERROR("gem install " + pkg.name + " 失败");
        return {false, "", "gem 安装失败"};
    }
}

Result GemBackend::remove(const Package& pkg) {
    std::string cmd = "gem uninstall -x -- " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已卸载: " + pkg.name);
        return {true, "已卸载: " + pkg.name, ""};
    } else {
        SSPM_ERROR("gem uninstall " + pkg.name + " 失败");
        return {false, "", "gem 卸载失败"};
    }
}

SearchResult GemBackend::search(const std::string& query) {
    std::string cmd = "gem search -- " + query;
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
        // 简单解析 gem search 输出格式
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t parenPos = line.find('(', spacePos);
            if (parenPos != std::string::npos) {
                std::string name = line.substr(0, spacePos);
                std::string version = line.substr(parenPos + 1);
                size_t closeParen = version.find(')');
                if (closeParen != std::string::npos) {
                    version = version.substr(0, closeParen);
                    packages.push_back({name, version, "", "gem", ""});
                }
            }
        }
    }

    return {packages, true, ""};
}

Result GemBackend::update() {
    // gem 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result GemBackend::upgrade() {
    std::string cmd = "gem update";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("所有 gem 已更新");
        return {true, "所有 gem 已更新", ""};
    } else {
        SSPM_ERROR("gem update 失败");
        return {false, "", "gem 更新失败"};
    }
}

PackageList GemBackend::list_installed() {
    std::string cmd = "gem list";
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
        // 解析格式：rails (7.0.0, 6.1.0)
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t parenPos = line.find('(', spacePos);
            if (parenPos != std::string::npos) {
                size_t closeParen = line.find(')', parenPos);
                if (closeParen != std::string::npos) {
                    std::string name = line.substr(0, spacePos);
                    std::string version = line.substr(parenPos + 1, closeParen - parenPos - 1);
                    // 取第一个版本
                    size_t commaPos = version.find(", ");
                    if (commaPos != std::string::npos) {
                        version = version.substr(0, commaPos);
                    }
                    packages.push_back({name, version, "", "gem", ""});
                }
            }
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> GemBackend::info(const std::string& name) {
    std::string cmd = "gem list -- ^" + name + "$";
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

    // 解析格式：rails (7.0.0, 6.1.0)
    size_t spacePos = output.find(' ');
    if (spacePos != std::string::npos) {
        size_t parenPos = output.find('(', spacePos);
        if (parenPos != std::string::npos) {
            size_t closeParen = output.find(')', parenPos);
            if (closeParen != std::string::npos) {
                std::string version = output.substr(parenPos + 1, closeParen - parenPos - 1);
                // 取第一个版本
                size_t commaPos = version.find(", ");
                if (commaPos != std::string::npos) {
                    version = version.substr(0, commaPos);
                }
                return Package{name, version, "", "gem", ""};
            }
        }
    }

    return std::nullopt;
}

} // namespace sspm::backends
