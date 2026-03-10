#pragma once
// ═══════════════════════════════════════════════════════════
//  layer/layer_manager.h — Backend 注册中心（v3.0）
//  25 个 backend 静态注册
// ═══════════════════════════════════════════════════════════
#include "backend.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace SSPM {

class LayerManager {
public:
    static void registerAll();

    // 获取 backend（不检查 available()）
    static LayerBackend* get(const std::string& name);

    // 获取 backend（只返回 available() 为 true 的）
    static LayerBackend* getAvailable(const std::string& name);

    // 获取默认 backend（当前系统可用）
    static LayerBackend* getDefault();

    // 所有可用 backend 名称
    static std::vector<std::string> availableNames();

    // 所有注册的 backend 名称（不论是否可用）
    static std::vector<std::string> allNames();

private:
    static std::map<std::string, std::unique_ptr<LayerBackend>> registry_;
};

} // namespace SSPM
