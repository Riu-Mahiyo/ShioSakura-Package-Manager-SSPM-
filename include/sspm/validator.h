#pragma once
#include <string>
#include <vector>

namespace sspm {

// ── Input Validator ───────────────────────────────────────────────────────────
//
// Strict input validation for all package names, paths, and versions.
//
// Philosophy: prevent injection attacks, path traversal, and other
// malicious input before they reach the execution engine.
class InputValidator {
public:
    struct Result {
        bool        ok     = false;
        std::string value;
        std::string reason;
        explicit operator bool() const { return ok; }
    };

    // Validates a package name: [a-zA-Z0-9._+-] and not starting with '-'
    static Result pkg(const std::string& name);

    // Validates a list of package names
    static Result pkg_list(const std::vector<std::string>& names);

    // Validates a path (checks for null bytes, path traversal, etc.)
    static Result file_path(const std::string& path,
                            bool must_be_absolute = false,
                            bool must_exist      = false);

    // Validates a version string: [a-zA-Z0-9._+~:-]
    static Result version(const std::string& ver);

    // Checks if a string contains shell metacharacters
    static bool has_shell_metachars(const std::string& s);

    // Validates an executable path (checks against safe prefixes)
    static Result executable_path(const std::string& path);

    // Validates a string's safety (no control characters, null bytes, etc.)
    static Result safe_string(const std::string& s, size_t max_len = 4096);
};

} // namespace sspm
