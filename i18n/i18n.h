#pragma once
// ═══════════════════════════════════════════════════════════
//  i18n/i18n.h — 多语言系统（清单第 9 节）
//  设计：
//    - JSON 语言包（/usr/share/sspm/i18n/<lang>.json）
//    - 自动语言检测（LANG / LC_ALL 环境变量）
//    - 强制语言参数（--lang=zh）
//    - 带 fallback：找不到翻译 → 返回英文 key
// ═══════════════════════════════════════════════════════════
#include <string>
#include <map>

namespace SSPM {

class I18n {
public:
    // 初始化（在 main() 早期调用）
    // lang 为空时自动检测 LANG 环境变量
    static void init(const std::string& lang = "");

    // 翻译函数（找不到 → 返回 key 本身）
    static std::string tr(const std::string& key);

    // 当前语言
    static std::string currentLang();

    // 检查语言包是否存在
    static bool hasLang(const std::string& lang);

    // 语言包搜索路径
    static std::string langFilePath(const std::string& lang);

private:
    static std::string               lang_;
    static std::map<std::string, std::string> strings_;

    static bool loadLangFile(const std::string& path);
    static std::string detectSystemLang();
};

// 简写宏（方便在代码里用 _("key")）
#define _(key) SSPM::I18n::tr(key)

} // namespace SSPM
