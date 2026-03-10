#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <filesystem>
#include "sspm/plugin.h"
#include "sspm/backend_registry.h"

// sspm CLI entry point
// Routes commands to the appropriate subsystem

namespace fs = std::filesystem;

namespace sspm::cli {

void cmd_install(const std::vector<std::string>& args);
void cmd_remove(const std::vector<std::string>& args);
void cmd_search(const std::vector<std::string>& args);
void cmd_update(const std::vector<std::string>& args);
void cmd_upgrade(const std::vector<std::string>& args);
void cmd_list(const std::vector<std::string>& args);
void cmd_info(const std::vector<std::string>& args);
void cmd_doctor(const std::vector<std::string>& args);
void cmd_repo(const std::vector<std::string>& args);
void cmd_cache(const std::vector<std::string>& args);
void cmd_config(const std::vector<std::string>& args);
void cmd_profile(const std::vector<std::string>& args);
void cmd_history(const std::vector<std::string>& args);
void cmd_rollback(const std::vector<std::string>& args);
void cmd_verify(const std::vector<std::string>& args);
void cmd_mirror(const std::vector<std::string>& args);
void cmd_log(const std::vector<std::string>& args);
void cmd_daemon(const std::vector<std::string>& args);
void cmd_plugin(const std::vector<std::string>& args);
void cmd_build(const std::vector<std::string>& args);
void cmd_backends(const std::vector<std::string>& args);
void cmd_lock(const std::vector<std::string>& args);
void cmd_amber_token(const std::vector<std::string>& args);
void cmd_fix_path(const std::vector<std::string>& args);
void cmd_version(const std::vector<std::string>& args);

} // namespace sspm::cli

int main(int argc, char* argv[]) {
    using namespace sspm::cli;

    // Initialize plugins
    std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
    std::string plugin_dir = home + "/.local/share/sspm/plugins";
    sspm::plugin::PluginManager plugin_mgr(plugin_dir);
    plugin_mgr.load_all();

    if (argc < 2) {
        std::cout << "Usage: sspm <command> [options]\n";
        std::cout << "Run 'sspm --help' for available commands.\n";
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "--version" || cmd == "-v" || cmd == "version") {
        cmd_version({});
        return 0;
    }

    std::vector<std::string> args(argv + 2, argv + argc);

    // Command dispatch table
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> dispatch = {
        { "install",  cmd_install  },
        { "remove",   cmd_remove   },
        { "search",   cmd_search   },
        { "update",   cmd_update   },
        { "upgrade",  cmd_upgrade  },
        { "list",     cmd_list     },
        { "info",     cmd_info     },
        { "doctor",   cmd_doctor   },
        { "repo",     cmd_repo     },
        { "cache",    cmd_cache    },
        { "config",   cmd_config   },
        { "profile",  cmd_profile  },
        { "history",  cmd_history  },
        { "rollback", cmd_rollback },
        { "verify",   cmd_verify   },
        { "mirror",   cmd_mirror   },
        { "log",      cmd_log      },
        { "daemon",   cmd_daemon   },
        { "plugin",   cmd_plugin   },
        { "build",    cmd_build    },
        { "backends", cmd_backends },
        { "lock",     cmd_lock     },
        { "amber-token", cmd_amber_token },
        { "fix-path", cmd_fix_path },
        { "version",  cmd_version  },
    };

    auto it = dispatch.find(cmd);
    if (it != dispatch.end()) {
        it->second(args);
    } else {
        std::cerr << "sspm: unknown command '" << cmd << "'\n";
        std::cerr << "Run 'sspm --help' for available commands.\n";
        return 1;
    }

    return 0;
}
