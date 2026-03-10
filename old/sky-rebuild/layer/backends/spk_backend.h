#pragma once
#include "../backend.h"

namespace SSPM {

// spkg — Sky 自有包格式（类 tar.xz + 元数据）
// 格式：<name>-<version>-<arch>.spkg（本质是 tar.xz 加 MANIFEST）
// 支持：Linux / macOS / FreeBSD（跨平台）
// 用途：sky make --spkg 打包；sspm install <file>.spkg 安装
class SpkBackend : public LayerBackend {
public:
    std::string name() const override { return "spk"; }
    bool available() const override { return true; }  // 内置，始终可用

    // pkgs 可以是 .spkg 文件路径 或 包名（从 spkg repo 拉取）
    BackendResult install(const std::vector<std::string>& pkgs) override;
    BackendResult remove(const std::vector<std::string>& pkgs)  override;
    BackendResult upgrade()                                      override;
    BackendResult search(const std::string& query)               override;
    std::string   installedVersion(const std::string& pkg)       override;

    // spkg 特有
    static bool        pack(const std::string& srcDir,
                             const std::string& outputFile);
    static bool        verify(const std::string& spkgFile);    // 校验完整性
    static std::string spkgDir();                               // 安装根目录
    static std::string dbDir();                                 // spkg 元数据目录

private:
    static BackendResult installFile(const std::string& path);
    static std::string   readManifestField(const std::string& spkgFile,
                                            const std::string& field);
};

} // namespace SSPM
