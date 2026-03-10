#pragma once
#include <vector>
#include <string>
#include <cstdlib>

// Helper to get home dir
inline std::string get_home() {
    const char* home = getenv("HOME");
    return home ? std::string(home) : "/tmp";
}

// Default runtime paths (resolved at startup from config/env)
inline const std::string DB_PATH    = get_home() + "/.local/share/sspm/sky.db";
inline const std::string CACHE_PATH = get_home() + "/.cache/sspm";
inline const std::string LOG_PATH   = get_home() + "/.local/state/sspm/log/sspm.log";
inline const std::string CFG_PATH   = get_home() + "/.config/sspm/config.yaml";
inline const std::string PLUGIN_DIR = get_home() + "/.local/share/sspm/plugins";

namespace sspm::cli {

void cmd_install (const std::vector<std::string>& args);
void cmd_remove  (const std::vector<std::string>& args);
void cmd_search  (const std::vector<std::string>& args);
void cmd_update  (const std::vector<std::string>& args);
void cmd_upgrade (const std::vector<std::string>& args);
void cmd_list    (const std::vector<std::string>& args);
void cmd_info    (const std::vector<std::string>& args);
void cmd_history (const std::vector<std::string>& args);
void cmd_rollback(const std::vector<std::string>& args);
void cmd_verify  (const std::vector<std::string>& args);
void cmd_repo    (const std::vector<std::string>& args);
void cmd_cache   (const std::vector<std::string>& args);
void cmd_config  (const std::vector<std::string>& args);
void cmd_profile (const std::vector<std::string>& args);
void cmd_mirror  (const std::vector<std::string>& args);
void cmd_log     (const std::vector<std::string>& args);
void cmd_daemon  (const std::vector<std::string>& args);
void cmd_plugin  (const std::vector<std::string>& args);
void cmd_build   (const std::vector<std::string>& args);
void cmd_doctor  (const std::vector<std::string>& args);
void cmd_backends(const std::vector<std::string>& args);
void cmd_lock    (const std::vector<std::string>& args);
void cmd_amber_token(const std::vector<std::string>& args);
void cmd_fix_path(const std::vector<std::string>& args);
void cmd_version (const std::vector<std::string>& args);

} // namespace sspm::cli
