#include "sspm/validator.h"
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace sspm {

static const char kShellMetachars[] = " \t\n\r;|&$`\\\"'<>(){}[]!#~*?^=%@";

bool InputValidator::has_shell_metachars(const std::string& s) {
    for (char c : s) {
        for (const char* p = kShellMetachars; *p; ++p) {
            if (c == *p) return true;
        }
    }
    return false;
}

InputValidator::Result InputValidator::pkg(const std::string& name) {
    if (name.empty())
        return {false, "", "Package name cannot be empty"};

    if (name.size() > 255)
        return {false, "", "Package name too long"};

    if (name.find("..") != std::string::npos)
        return {false, "", "Package name contains path traversal"};

    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != '.' && c != '_' && c != '+' && c != '-') {
            return {false, "", "Package name contains illegal characters"};
        }
    }

    if (name[0] == '-')
        return {false, "", "Package name cannot start with '-'"};

    return {true, name, ""};
}

InputValidator::Result InputValidator::pkg_list(const std::vector<std::string>& names) {
    if (names.empty())
        return {false, "", "Package list is empty"};
    for (const auto& n : names) {
        auto r = pkg(n);
        if (!r) return r;
    }
    return {true, "", ""};
}

InputValidator::Result InputValidator::file_path(
    const std::string& path, bool must_be_absolute, bool must_exist)
{
    if (path.empty())
        return {false, "", "Path cannot be empty"};
    if (path.find('\0') != std::string::npos)
        return {false, "", "Path contains null byte"};

    try {
        fs::path p(path);
        for (const auto& comp : p) {
            if (comp == "..") {
                return {false, "", "Path contains traversal '..'"};
            }
        }
    } catch (...) {
        return {false, "", "Path parsing failed"};
    }

    if (must_be_absolute && path[0] != '/')
        return {false, "", "Path must be absolute"};

    if (must_exist && !fs::exists(path))
        return {false, "", "Path does not exist"};

    return {true, path, ""};
}

InputValidator::Result InputValidator::version(const std::string& ver) {
    if (ver.empty())
        return {false, "", "Version cannot be empty"};
    if (ver.size() > 128)
        return {false, "", "Version too long"};

    for (char c : ver) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != '.' && c != '_' && c != '+' &&
            c != '~' && c != ':' && c != '-') {
            return {false, "", "Version contains illegal characters"};
        }
    }
    return {true, ver, ""};
}

InputValidator::Result InputValidator::executable_path(const std::string& path) {
    auto res = file_path(path, true, true);
    if (!res) return res;

    static const char* const kSafeExecPrefixes[] = {
        "/usr/bin/", "/usr/sbin/", "/bin/", "/sbin/",
        "/usr/local/bin/", "/usr/local/sbin/",
        "/opt/homebrew/bin/",
        "/nix/store/",
        "/run/current-system/sw/bin/",
        nullptr
    };

    bool in_prefix = false;
    for (auto p = kSafeExecPrefixes; *p; ++p) {
        if (path.rfind(*p, 0) == 0) { in_prefix = true; break; }
    }
    if (!in_prefix)
        return {false, "", "Executable path not in safe prefixes"};

    return {true, path, ""};
}

InputValidator::Result InputValidator::safe_string(const std::string& s, size_t max_len) {
    if (s.size() > max_len)
        return {false, "", "String too long"};
    for (char c : s) {
        if ((unsigned char)c < 0x20 && c != '\t' && c != '\n')
            return {false, "", "String contains control characters"};
        if (c == '\0')
            return {false, "", "String contains null byte"};
    }
    return {true, s, ""};
}

} // namespace sspm
