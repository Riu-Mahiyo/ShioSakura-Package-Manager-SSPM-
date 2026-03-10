// layer/backends/wine_backend.cpp — Wine + Winetricks Backend
#include "wine_backend.h"
#include "sspm/log.h"
#include <sstream>

namespace sspm::backends {

bool WineBackend::wineAvailable() {
    return system("which wine > /dev/null 2>&1") == 0 || 
           system("which wine64 > /dev/null 2>&1") == 0;
}

bool WineBackend::winetricksAvailable() {
    return system("which winetricks > /dev/null 2>&1") == 0;
}

std::string WineBackend::wineVersion() {
    // 尝试 wine --version
    FILE* pipe = popen("wine --version", "r");
    if (!pipe) {
        // 尝试 wine64 --version
        pipe = popen("wine64 --version", "r");
        if (!pipe) return "";
    }

    char buffer[128];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    // 提取版本号
    size_t nl = output.find('\n');
    if (nl != std::string::npos) {
        output = output.substr(0, nl);
    }
    return output;
}

bool WineBackend::is_available() const {
    return wineAvailable();
}

Result WineBackend::install(const Package& pkg) {
    if (!winetricksAvailable()) {
        SSPM_ERROR("未找到 winetricks，请先安装: sspm install winetricks");
        return {false, "", "winetricks not found"};
    }

    std::string cmd = "WINEDEBUG=-all winetricks " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("Wine 组件安装成功");
        return {true, "Wine 组件安装成功", ""};
    } else {
        SSPM_ERROR("winetricks 失败 exit=" + std::to_string(rc));
        return {false, "", "winetricks 安装失败"};
    }
}

Result WineBackend::remove(const Package& pkg) {
    // winetricks 不支持卸载单个 verb
    // 只能通过 winetricks annihilate 清空整个 Wine prefix
    SSPM_WARN("Wine 组件不支持单独卸载");
    SSPM_INFO("若需重置 Wine 环境，运行: winetricks annihilate");
    return {true, "", ""};
}

SearchResult WineBackend::search(const std::string& query) {
    if (!winetricksAvailable()) {
        return {{}, false, "winetricks not found"};
    }

    std::string cmd = "winetricks list-all";
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

    // 过滤含 query 的行
    PackageList packages;
    std::istringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find(query) != std::string::npos) {
            packages.push_back({line, "", "", "wine", ""});
        }
    }

    if (packages.empty()) {
        SSPM_INFO("未找到匹配的 Wine 组件");
    }

    return {packages, true, ""};
}

Result WineBackend::update() {
    // wine 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result WineBackend::upgrade() {
    std::string cmd = "winetricks --self-update";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("winetricks 已更新");
        return {true, "winetricks 已更新", ""};
    } else {
        SSPM_WARN("winetricks --self-update 失败（可能已是最新）");
        return {true, "", ""};  // 非致命
    }
}

PackageList WineBackend::list_installed() {
    // wine 组件没有明确的列表概念，返回空列表
    return {};
}

std::optional<Package> WineBackend::info(const std::string& name) {
    // 返回 wine 版本（组件本身没有版本概念）
    std::string version = wineVersion();
    if (!version.empty()) {
        return Package{name, version, "", "wine", ""};
    }
    return std::nullopt;
}

} // namespace sspm::backends
