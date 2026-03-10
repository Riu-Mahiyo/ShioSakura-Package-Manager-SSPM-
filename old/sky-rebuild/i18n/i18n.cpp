// ═══════════════════════════════════════════════════════════
//  i18n/i18n.cpp — 多语言实现
//  JSON 格式极简解析（不依赖第三方库）
//  格式：{"key": "translated string", ...}
// ═══════════════════════════════════════════════════════════
#include "i18n.h"
#include "../core/logger.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <vector>

namespace SSPM {
namespace fs = std::filesystem;

std::string               I18n::lang_;
std::map<std::string, std::string> I18n::strings_;

std::string I18n::detectSystemLang() {
    // 优先级：LC_ALL > LANG > LC_MESSAGES
    const char* lang = getenv("LC_ALL");
    if (!lang || !*lang) lang = getenv("LANG");
    if (!lang || !*lang) lang = getenv("LC_MESSAGES");
    if (!lang || !*lang) return "en";

    std::string s(lang);
    // "zh_CN.UTF-8" → "zh_CN" → "zh"
    auto dot = s.find('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    auto under = s.find('_');
    // 返回带地区码的版本，fallback 会尝试 "zh"
    return s;
}

std::string I18n::langFilePath(const std::string& lang) {
    // 搜索路径
    static const char* kDirs[] = {
        "/usr/share/sspm/i18n/",
        "/usr/local/share/sspm/i18n/",
        nullptr
    };
    // 也检查 XDG_DATA_HOME
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string userDir = xdg ? std::string(xdg) + "/sspm/i18n/" :
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.local/share/sspm/i18n/";

    std::vector<std::string> dirs;
    dirs.push_back(userDir);
    for (auto d = kDirs; *d; ++d) dirs.push_back(*d);

    for (auto& dir : dirs) {
        std::string path = dir + lang + ".json";
        if (fs::exists(path)) return path;
    }
    return "";
}

bool I18n::loadLangFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

    // 极简 JSON 解析（只处理 "key": "value" 对）
    strings_.clear();
    size_t pos = 0;
    while (pos < content.size()) {
        auto k1 = content.find('"', pos);
        if (k1 == std::string::npos) break;
        auto k2 = content.find('"', k1 + 1);
        if (k2 == std::string::npos) break;
        std::string key = content.substr(k1 + 1, k2 - k1 - 1);

        auto colon = content.find(':', k2 + 1);
        if (colon == std::string::npos) break;

        auto v1 = content.find('"', colon + 1);
        if (v1 == std::string::npos) break;

        // 处理转义字符（\"）
        std::string val;
        size_t i = v1 + 1;
        while (i < content.size()) {
            char c = content[i];
            if (c == '\\' && i + 1 < content.size()) {
                char next = content[i + 1];
                switch (next) {
                    case '"':  val += '"';  i += 2; continue;
                    case '\\': val += '\\'; i += 2; continue;
                    case 'n':  val += '\n'; i += 2; continue;
                    case 't':  val += '\t'; i += 2; continue;
                    default:   val += c;   i++;    continue;
                }
            }
            if (c == '"') { i++; break; }
            val += c;
            i++;
        }

        if (!key.empty()) strings_[key] = val;
        pos = i;
    }

    return !strings_.empty();
}

void I18n::init(const std::string& lang) {
    std::string targetLang = lang.empty() ? detectSystemLang() : lang;

    // 先尝试完整语言代码（如 zh_CN），再尝试短代码（如 zh）
    std::string path = langFilePath(targetLang);
    if (path.empty()) {
        auto under = targetLang.find('_');
        if (under != std::string::npos) {
            path = langFilePath(targetLang.substr(0, under));
        }
    }

    if (!path.empty() && loadLangFile(path)) {
        lang_ = targetLang;
        Logger::debug("i18n: 已加载语言包 " + path +
                      " (" + std::to_string(strings_.size()) + " 条)");
    } else {
        lang_ = "en";
        // 英文不需要语言包，直接用 key
    }
}

std::string I18n::tr(const std::string& key) {
    auto it = strings_.find(key);
    return (it != strings_.end()) ? it->second : key;
}

std::string I18n::currentLang() { return lang_; }

bool I18n::hasLang(const std::string& lang) {
    return !langFilePath(lang).empty();
}

} // namespace SSPM
