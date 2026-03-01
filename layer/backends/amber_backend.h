#pragma once
// ═══════════════════════════════════════════════════════════
//  layer/backends/amber_backend.h — Amber PM Backend
//  官方站点：https://amber-pm.spark-app.store
//
//  架构文档要求：
//    ✓ token 机制（Bearer auth）
//    ✓ JSON 解析（下载包元数据）
//    ✓ 下载 + SHA256 校验
//    ✓ 安装失败自动 rollback
//    ✓ SkyDB 记录
//
//  Amber PM 是独立包格式，安装到 ~/.local/share/amber/ 或系统目录
//  官方 CLI：amber（检查存在性）
// ═══════════════════════════════════════════════════════════
#include "../backend.h"
#include <string>

namespace SSPM {

struct AmberPkgMeta {
    std::string name;
    std::string version;
    std::string downloadUrl;
    std::string sha256;
    std::string description;
};

class AmberBackend : public LayerBackend {
public:
    std::string name() const override { return "amber"; }
    bool available() const override;

    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // Amber 特有 API
    static std::string     apiBase();
    static std::string     token();               // 读取 ~/.config/sspm/amber.token
    static bool            setToken(const std::string& tok);  // 写入 token

    // 查询包元数据（通过 Amber API）
    static AmberPkgMeta    fetchMeta(const std::string& pkg);
    // 下载并校验包
    static bool            downloadAndVerify(const AmberPkgMeta& meta,
                                             const std::string& destPath);
    // 本地安装已下载的包
    static BackendResult   installFile(const std::string& path);
};

} // namespace SSPM
