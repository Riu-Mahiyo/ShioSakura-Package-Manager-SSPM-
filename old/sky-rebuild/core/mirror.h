#pragma once
// ═══════════════════════════════════════════════════════════
//  core/mirror.h — 智能镜像源选择器
//  架构文档第六节：跨发行版兼容策略
//
//  功能：
//    1. 通过 IP 地理位置自动选择最近镜像
//    2. 内置区域→镜像映射（apt/dnf/pacman/pip/npm）
//    3. 只修改 SSPM 管理的源，用户自定义源不可动
//    4. 支持手动覆盖（--mirror=cn/us/eu/...）
//
//  支持的区域：CN JP KR DE US UK FR AU CA SG
// ═══════════════════════════════════════════════════════════
#include <string>
#include <map>
#include <vector>

namespace SSPM {

struct MirrorEntry {
    std::string url;
    std::string region;  // "CN", "US", "EU", ...
    int         latencyMs = 0;   // 测速后填入
};

// 每个 backend 对应的镜像配置
struct BackendMirrors {
    std::vector<MirrorEntry> mirrors;
    std::string              selectedUrl;
    bool                     userManaged = false;  // 用户自定义，禁止修改
};

class MirrorManager {
public:
    // 获取单例
    static MirrorManager& instance();

    // ── 区域检测 ──────────────────────────────────────────
    // 通过 IP 地理位置检测用户所在区域
    // 优先级：环境变量 SSPM_REGION > IP查询 > 默认 "US"
    std::string detectRegion();

    // 手动设置区域（--mirror=cn 等）
    void setRegion(const std::string& region);

    // 获取当前区域
    std::string currentRegion() const { return region_; }

    // ── 镜像选择 ──────────────────────────────────────────
    // 为指定 backend 获取最优镜像 URL
    std::string getBestMirror(const std::string& backend);

    // ── 镜像应用 ──────────────────────────────────────────
    // 应用镜像配置到系统（只在首次安装或用户明确同意时执行）
    bool applyMirror(const std::string& backend);

    // 检查是否已应用 SSPM 管理的镜像（vs 用户自定义）
    bool isSSPMManaged(const std::string& backend);

    // ── 内置镜像数据库 ───────────────────────────────────
    // 区域 → backend → mirrors
    static const std::map<std::string,
                 std::map<std::string, std::string>>& builtinMirrors();

private:
    MirrorManager() = default;
    std::string region_;

    // IP 查询（调用 ip-api.com 或 ipinfo.io，离线则返回""）
    std::string queryIpRegion();

    // 映射 ISO 国家码 → 我们的区域标识
    static std::string normalizeRegion(const std::string& countryCode);
};

} // namespace SSPM
