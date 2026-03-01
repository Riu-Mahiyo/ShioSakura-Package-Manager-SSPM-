#pragma once
// ═══════════════════════════════════════════════════════════
//  profile/profile.h — 配置 Profile 系统
//  架构文档第十节：进阶能力
//    ✓ 配置 profile 系统
//    ✓ declarative install 模式（类似 nix 思路）
//    ✓ 可复现安装配置导出
//
//  Profile 文件格式（TOML-like，自实现解析）：
//    [meta]
//    name = "dev-workstation"
//    version = "1.0"
//
//    [packages]
//    apt = ["curl", "git", "vim", "htop"]
//    pip = ["black", "pytest", "requests"]
//    npm = ["typescript", "eslint"]
//    cargo = ["ripgrep", "bat", "fd-find"]
//
//    [settings]
//    mirror_region = "CN"
//    prefer_backend = ["apt", "flatpak"]
//    allow_root = true
//
//  用法：
//    sspm profile apply dev-workstation.profile
//    sspm profile export > current.profile
//    sspm profile diff current.profile target.profile
// ═══════════════════════════════════════════════════════════
#include <string>
#include <vector>
#include <map>

namespace SSPM {

struct ProfilePkg {
    std::string name;
    std::string version;    // 空 = latest
    std::string backend;
    bool        optional = false;
};

struct ProfileSettings {
    std::string              mirrorRegion;
    std::vector<std::string> preferBackend;
    bool                     allowRoot    = false;
    bool                     dryRunFirst  = true;   // 默认先 dry-run 预览
};

struct Profile {
    std::string name;
    std::string version;
    std::string description;
    std::string author;

    // backend → packages
    std::map<std::string, std::vector<std::string>> packages;

    ProfileSettings settings;
};

class ProfileManager {
public:
    // 加载 profile 文件
    static Profile load(const std::string& path);

    // 保存 profile 到文件
    static bool save(const Profile& profile, const std::string& path);

    // 导出当前系统安装状态为 profile
    static Profile exportCurrent(const std::string& name = "exported");

    // 应用 profile（declarative install）
    // 返回：执行的操作数
    static int apply(const Profile& profile, bool dryRun = false);

    // 对比两个 profile 的差异
    struct Diff {
        std::vector<ProfilePkg> toInstall;
        std::vector<ProfilePkg> toRemove;
        std::vector<ProfilePkg> unchanged;
    };
    static Diff diff(const Profile& current, const Profile& target);

    // 列出已保存的 profile
    static std::vector<std::string> listProfiles();

    // 用户 profile 目录
    static std::string profileDir();

private:
    static Profile parseToml(const std::string& content);
    static std::string dumpToml(const Profile& profile);
};

} // namespace SSPM
