#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace sspm {

// ── Execution Result ──────────────────────────────────────────────────────────
struct ExecResult {
    int         exit_code    = 0;
    std::string stdout_out;
    std::string stderr_out;
    double      duration_sec = 0.0;
    bool        timed_out    = false;

    bool ok() const { return exit_code == 0 && !timed_out; }
    std::string diagnose() const;
};

// ── Execution Options ─────────────────────────────────────────────────────────
struct ExecOptions {
    bool capture_output = true;
    bool merge_stderr   = false;
    int  timeout_sec    = 60;
    int  kill_grace_sec = 3;
    std::string work_dir;
    std::map<std::string, std::string> extra_env;
};

// ── Execution Engine ──────────────────────────────────────────────────────────
//
// Safe wrapper around fork/execvp.
// NEVER uses 'sh -c' or 'system()'.
// Arguments are passed as a vector to prevent shell injection.
class ExecEngine {
public:
    using Options = ExecOptions;

    // Run command, piping output to current stdout/stderr (e.g. for installers)
    static int run(const std::string& executable,
                   const std::vector<std::string>& args,
                   const Options& opts = {});

    // Run command and capture its output
    static ExecResult capture(const std::string& executable,
                              const std::vector<std::string>& args,
                              const Options& opts = {});

    // Run command and capture the first line of output
    static std::string capture_line(const std::string& executable,
                                    const std::vector<std::string>& args,
                                    const Options& opts = {});

    // Check if an executable exists in PATH
    static bool exists(const std::string& executable);

    // Resolve an executable name to its absolute path in PATH
    static std::string find_in_path(const std::string& name);

private:
    static ExecResult do_exec(const std::string& executable,
                              const std::vector<std::string>& args,
                              const Options& opts);

    static void audit_log(const std::string& executable,
                          const std::vector<std::string>& args);

    static const char* const kAllowedPrefixes[];
};

} // namespace sspm
