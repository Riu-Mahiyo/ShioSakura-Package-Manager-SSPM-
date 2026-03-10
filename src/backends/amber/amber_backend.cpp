// ═══════════════════════════════════════════════════════════
//  layer/backends/amber_backend.cpp — Amber PM Backend
//  架构文档第 6 节：Amber PM 集成
//
//  官方网站：https://amber-pm.spark-app.store
//  安装路径：/usr/local/bin/amber（系统级）
//            ~/.local/bin/amber（用户级）
//
//  工作流程（架构文档规定）：
//    1. 读取 token（~/.config/sspm/amber.token）
//    2. 调用 amber CLI 执行操作（API 通过官方工具封装）
//       OR 直接调用 Amber HTTP API（如果 CLI 不可用）
//    3. 校验下载包（SHA256）
//    4. 安装失败 → rollback（删除已解压文件）
//    5. 写入 SkyDB（由 cli_router 统一处理）
//
//  注意：Amber PM 的 CLI 会话最终以 amber install <pkg> 形式执行
//  如果 amber CLI 不存在，显示安装指引
// ═══════════════════════════════════════════════════════════
#include "amber_backend.h"
#include "sspm/backend_registry.h"
#include "sspm/log.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sspm::backends {
namespace fs = std::filesystem;

// ── Amber API base URL（来自架构文档）────────────────────
std::string AmberBackend::apiBase() {
    const char* env = getenv("AMBER_API");
    if (env && *env) return std::string(env);
    return "https://amber-pm.spark-app.store/api/v1";
}

// ── Token 管理 ────────────────────────────────────────────
std::string AmberBackend::token() {
    const char* home = getenv("HOME");
    if (!home) return "";

    // 优先环境变量
    const char* envtok = getenv("AMBER_TOKEN");
    if (envtok && *envtok) return std::string(envtok);

    // 读文件
    std::string path = std::string(home) + "/.config/sspm/amber.token";
    std::ifstream f(path);
    if (!f) return "";
    std::string tok;
    std::getline(f, tok);
    // trim whitespace
    while (!tok.empty() && (tok.back() == '\n' || tok.back() == '\r' || tok.back() == ' '))
        tok.pop_back();
    return tok;
}

bool AmberBackend::setToken(const std::string& tok) {
    const char* home = getenv("HOME");
    if (!home) return false;
    std::string dir  = std::string(home) + "/.config/sspm";
    std::string path = dir + "/amber.token";
    try { fs::create_directories(dir); } catch (...) { return false; }
    std::ofstream f(path);
    if (!f) return false;
    f << tok << "\n";
    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write);
    SSPM_INFO("Amber token 已保存到 " + path);
    return true;
}

// ── 可用性检测 ────────────────────────────────────────────
bool AmberBackend::is_available() const {
    // amber CLI 存在
    return system("which amber > /dev/null 2>&1") == 0;
}

// ── 包元数据（stub：实际通过 amber CLI 查询）────────────
AmberPkgMeta AmberBackend::fetchMeta(const std::string& pkg) {
    // 通过 amber info <pkg> --json 获取元数据
    std::string cmd = "amber info --json -- " + pkg;
    SSPM_INFO("执行: " + cmd);

    // 执行命令并捕获输出
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {pkg, "", "", "", ""};
    }

    std::string output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    AmberPkgMeta meta;
    meta.name = pkg;

    if (output.empty()) return meta;

    // 解析简单 JSON（只提取 version 和 sha256）
    auto extractField = [&](const std::string& key) -> std::string {
        auto pos = output.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        pos = output.find('"', pos + key.size() + 3);
        if (pos == std::string::npos) return "";
        auto end = output.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return output.substr(pos + 1, end - pos - 1);
    };

    meta.version     = extractField("version");
    meta.sha256      = extractField("sha256");
    meta.downloadUrl = extractField("download_url");
    meta.description = extractField("description");
    return meta;
}

bool AmberBackend::downloadAndVerify(const AmberPkgMeta& meta,
                                      const std::string& destPath) {
    if (meta.downloadUrl.empty()) {
        SSPM_ERROR("[amber] 无法获取下载 URL");
        return false;
    }

    SSPM_INFO("下载: " + meta.downloadUrl);
    // amber CLI 负责实际下载；我们通过 curl 验证可达性
    // 实际下载由 amber install 命令处理
    // 这里只做连通性检测
    std::string cmd = "curl -fsSL -o " + destPath + " -- " + meta.downloadUrl;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc != 0) {
        SSPM_ERROR("[amber] 下载失败 exit=" + std::to_string(rc));
        return false;
    }

    // SHA256 校验
    if (!meta.sha256.empty()) {
        SSPM_INFO("校验 SHA256...");
        std::string shaCmd = "sha256sum -- " + destPath;
        SSPM_INFO("执行: " + shaCmd);

        FILE* pipe = popen(shaCmd.c_str(), "r");
        if (pipe) {
            char buffer[128];
            std::string output;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }
            pclose(pipe);

            std::string actual = output.substr(0, 64);
            if (actual == meta.sha256) {
                SSPM_INFO("SHA256 校验通过");
            } else {
                SSPM_ERROR("[amber] SHA256 不匹配！");
                SSPM_ERROR("  期望: " + meta.sha256);
                SSPM_ERROR("  实际: " + actual);
                fs::remove(destPath);
                return false;
            }
        } else {
            SSPM_WARN("sha256sum 不可用，跳过校验");
        }
    }
    return true;
}

Result AmberBackend::installFile(const std::string& path) {
    std::string cmd = "amber install -- " + path;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        return {true, "Amber 包安装成功", ""};
    } else {
        return {false, "", "amber install 失败"};
    }
}

// ── 主操作 ────────────────────────────────────────────────

Result AmberBackend::install(const Package& pkg) {
    // 提示 token（如果未设置）
    std::string tok = token();
    if (tok.empty()) {
        SSPM_WARN("Amber token 未设置");
        SSPM_INFO("请访问 https://amber-pm.spark-app.store 获取 token");
        SSPM_INFO("然后运行: sspm amber-token <your-token>");
    }

    std::string cmd = "amber install -- " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    // 设置 token 环境变量（amber CLI 会读取）
    if (!tok.empty()) {
        setenv("AMBER_TOKEN", tok.c_str(), 1);
    }

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已安装: " + pkg.name);
        return {true, "已安装: " + pkg.name, ""};
    } else {
        SSPM_ERROR("amber install " + pkg.name + " 失败 exit=" + std::to_string(rc));
        return {false, "", "amber 安装失败"};
    }
}

Result AmberBackend::remove(const Package& pkg) {
    std::string cmd = "amber remove -- " + pkg.name;
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("已卸载: " + pkg.name);
        return {true, "已卸载: " + pkg.name, ""};
    } else {
        SSPM_ERROR("amber remove " + pkg.name + " 失败");
        return {false, "", "amber 卸载失败"};
    }
}

SearchResult AmberBackend::search(const std::string& query) {
    std::string cmd = "amber search -- " + query;
    SSPM_INFO("执行: " + cmd);

    // 执行命令并捕获输出
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        SSPM_INFO("Amber PM 搜索: https://amber-pm.spark-app.store/search?q=" + query);
        return {{}, false, "（请访问官网搜索: https://amber-pm.spark-app.store）\n"};
    }

    std::string output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    if (output.empty()) {
        SSPM_INFO("Amber PM 搜索: https://amber-pm.spark-app.store/search?q=" + query);
        return {{}, false, "（请访问官网搜索: https://amber-pm.spark-app.store）\n"};
    }

    // 简单解析输出，提取包信息
    PackageList packages;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            packages.push_back({line, "", "", "amber", ""});
        }
    }

    return {packages, true, ""};
}

Result AmberBackend::update() {
    // amber 的 update 由 upgrade 处理
    return {true, "", ""};
}

Result AmberBackend::upgrade() {
    std::string cmd = "amber update";
    SSPM_INFO("执行: " + cmd);

    int rc = system(cmd.c_str());
    if (rc == 0) {
        SSPM_INFO("Amber 包更新完成");
        return {true, "Amber 包更新完成", ""};
    } else {
        SSPM_ERROR("amber update 失败 exit=" + std::to_string(rc));
        return {false, "", "amber 更新失败"};
    }
}

PackageList AmberBackend::list_installed() {
    std::string cmd = "amber list";
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
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        // 简单解析 amber list 输出格式
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            std::string name = line.substr(0, spacePos);
            std::string version = line.substr(spacePos + 1);
            // 去除换行符
            size_t nlPos = version.find('\n');
            if (nlPos != std::string::npos) {
                version = version.substr(0, nlPos);
            }
            packages.push_back({name, version, "", "amber", ""});
        }
    }
    pclose(pipe);

    return packages;
}

std::optional<Package> AmberBackend::info(const std::string& name) {
    std::string cmd = "amber info --version -- " + name;
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

    // 去除换行符
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }

    if (output.empty()) {
        return std::nullopt;
    }

    return Package{name, output, "", "amber", ""};
}

// Auto-register with the registry
struct AmberRegister {
    AmberRegister() {
        BackendRegistry::register_factory("amber", []() {
            return std::make_shared<AmberBackend>();
        });
    }
} _amber_register;

} // namespace sspm::backends
