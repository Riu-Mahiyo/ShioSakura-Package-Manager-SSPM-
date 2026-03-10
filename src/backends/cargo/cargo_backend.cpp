// layer/backends/cargo_backend.cpp — Rust cargo 包管理器
#include "cargo_backend.h"
#include "sspm/log.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <unistd.h>

namespace sspm::backends {
namespace fs = std::filesystem;

std::string CargoBackend::cargoBin() {
    // 优先 PATH 中的 cargo
    if (system("which cargo > /dev/null 2>&1") == 0) return "cargo";
    // 用户级安装路径
    const char* home = getenv("HOME");
    if (home) {
        std::string p = std::string(home) + "/.cargo/bin/cargo";
        if (fs::exists(p)) return p;
    }
    return "cargo";
}

bool CargoBackend::is_available() const {
    return system("which cargo > /dev/null 2>&1") == 0 || []{
        const char* h = getenv("HOME");
        return h && fs::exists(std::string(h) + "/.cargo/bin/cargo");
    }();
}

Result CargoBackend::install(const Package& pkg) {
    std::string cargo = cargoBin();
    std::string cmd = cargo + " install -- " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已安装: " + pkg.name);
        return {true, "已安装: " + pkg.name, ""};
    } else {
        SSPM_ERROR("cargo install " + pkg.name + " 失败 exit=" + std::to_string(rc));
        return {false, "", "cargo 安装失败"};
    }
}

Result CargoBackend::remove(const Package& pkg) {
    std::string cargo = cargoBin();
    std::string cmd = cargo + " uninstall -- " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已卸载: " + pkg.name);
        return {true, "已卸载: " + pkg.name, ""};
    } else {
        SSPM_ERROR("cargo uninstall " + pkg.name + " 失败");
        return {false, "", "cargo 卸载失败"};
    }
}

SearchResult CargoBackend::search(const std::string& query) {
    std::string cargo = cargoBin();
    std::string cmd = cargo + " search -- " + query;
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
        // 简单解析 cargo search 输出格式
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t versionPos = line.find(" v", spacePos);
            if (versionPos != std::string::npos) {
                size_t descPos = line.find(": ", versionPos);
                if (descPos != std::string::npos) {
                    std::string name = line.substr(0, spacePos);
                    std::string version = line.substr(versionPos + 2, descPos - versionPos - 2);
                    std::string description = line.substr(descPos + 2);
                    packages.push_back({name, version, description, "cargo", ""});
                }
            }
        }
    }

    return {packages, true, ""};
}

Result CargoBackend::update() {
    // cargo 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result CargoBackend::upgrade() {
    // cargo install-update 需要安装 cargo-update crate
    if (system("which cargo-install-update > /dev/null 2>&1") == 0) {
        std::string cmd = "cargo install-update -a";
        SSPM_INFO("执行: " + cmd);
        int rc = system(cmd.c_str());
        if (rc == 0) {
            return {true, "Cargo 包更新完成", ""};
        } else {
            return {false, "", "Cargo 更新失败"};
        }
    }

    // 回退：提示安装 cargo-update
    SSPM_WARN("升级全部 cargo 包需要 cargo-update crate");
    SSPM_INFO("安装方法: sspm install cargo-update --backend=cargo");
    SSPM_INFO("或手动: cargo install cargo-update");
    return {true, "", ""};
}

PackageList CargoBackend::list_installed() {
    std::string cargo = cargoBin();
    std::string cmd = cargo + " install --list";
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
        // 解析格式：ripgrep v13.0.0:
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            size_t versionPos = line.find(" v", spacePos);
            if (versionPos != std::string::npos) {
                size_t colonPos = line.find(':', versionPos);
                if (colonPos != std::string::npos) {
                    std::string name = line.substr(0, spacePos);
                    std::string version = line.substr(versionPos + 2, colonPos - versionPos - 2);
                    packages.push_back({name, version, "", "cargo", ""});
                }
            }
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> CargoBackend::info(const std::string& name) {
    std::string cargo = cargoBin();
    std::string cmd = cargo + " install --list";
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

    // 解析输出，查找指定包
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind(name + " ", 0) == 0) {
            // 格式：ripgrep v13.0.0:
            size_t versionPos = line.find(" v");
            size_t colonPos = line.find(':', versionPos);
            if (versionPos != std::string::npos && colonPos != std::string::npos) {
                std::string version = line.substr(versionPos + 2, colonPos - versionPos - 2);
                return Package{name, version, "", "cargo", ""};
            }
        }
    }

    return std::nullopt;
}

} // namespace sspm::backends
