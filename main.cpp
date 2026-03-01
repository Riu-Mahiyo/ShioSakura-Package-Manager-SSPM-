// ═══════════════════════════════════════════════════════════
//  main.cpp — SSPM 2.3.0 入口
//  流程：i18n 初始化 → OS 检测 → 注册 Backends → 分发命令
// ═══════════════════════════════════════════════════════════
#include "core/detect.h"
#include "core/logger.h"
#include "layer/layer_manager.h"
#include "cli/cli_router.h"
#include "i18n/i18n.h"

#include <string>
#include <cstring>

// 从 argv 中提取 --lang=xx 或 -L xx 参数
static std::string extractLang(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--lang=", 0) == 0) return a.substr(7);
        if ((a == "--lang" || a == "-L") && i + 1 < argc) return argv[i + 1];
    }
    return "";  // 空 = 自动检测
}

int main(int argc, char* argv[]) {
    // 1. 国际化初始化（读取 LANG 环境变量或 --lang 参数）
    std::string lang = extractLang(argc, argv);
    SSPM::I18n::init(lang);

    // 2. 系统检测（OS、arch、PATH 等，结果缓存）
    SSPM::Detect::run();

    // 3. 注册所有已编译 backend（21 个）
    SSPM::LayerManager::registerAll();

    // 4. 分发命令
    return SSPM::CliRouter::dispatch(argc, argv);
}
