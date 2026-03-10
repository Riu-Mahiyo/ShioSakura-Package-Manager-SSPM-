#include "sspm/exec.h"
#include "sspm/log.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <unordered_map>
#include <cassert>
#include <atomic>
#include <thread>

// POSIX
#include <unistd.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

namespace fs = std::filesystem;

namespace sspm {

// ── ExecResult ────────────────────────────────────────────────────────────────
std::string ExecResult::diagnose() const {
    if (timed_out) return "timeout";
    if (exit_code == 127) return "command not found (exit 127)";
    if (exit_code != 0)
        return "exit " + std::to_string(exit_code) +
               (stderr_out.empty() ? "" : ": " + stderr_out.substr(0, 200));
    return "success";
}

// ── ExecEngine ────────────────────────────────────────────────────────────────
const char* const ExecEngine::kAllowedPrefixes[] = {
    "/usr/bin/", "/usr/sbin/", "/bin/", "/sbin/",
    "/usr/local/bin/", "/usr/local/sbin/",
    "/opt/homebrew/bin/",
    "/nix/store/",
    "/run/current-system/sw/bin/",
    nullptr
};

void ExecEngine::audit_log(const std::string& executable,
                           const std::vector<std::string>& args)
{
    std::string cmd = "[exec] " + executable;
    for (const auto& a : args) cmd += " " + a;
    SSPM_DEBUG(cmd);
}

std::string ExecEngine::find_in_path(const std::string& name) {
    if (name.empty()) return "";
    if (name[0] == '/') return name;

    static std::unordered_map<std::string, std::string> cache;
    auto it = cache.find(name);
    if (it != cache.end()) return it->second;

    const char* path_env = getenv("PATH");
    if (!path_env) path_env = "/usr/bin:/bin:/usr/sbin:/sbin";
    std::string path_str(path_env);

    std::istringstream ss(path_str);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        std::string full = dir + "/" + name;
        if (access(full.c_str(), X_OK) == 0) {
            return cache[name] = full;
        }
    }
    return cache[name] = "";
}

bool ExecEngine::exists(const std::string& executable) {
    return !find_in_path(executable).empty();
}

ExecResult ExecEngine::do_exec(const std::string& executable,
                               const std::vector<std::string>& args,
                               const Options& opts)
{
    ExecResult result;
    auto t0 = std::chrono::steady_clock::now();

    std::string abs_exec = find_in_path(executable);
    if (abs_exec.empty()) {
        result.exit_code   = 127;
        result.stderr_out = "Command not found: " + executable;
        SSPM_ERROR("[exec] Command not found: " + executable);
        return result;
    }

    audit_log(abs_exec, args);

    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};
    if (opts.capture_output) {
        if (pipe(stdout_pipe) != 0) return { -1, "", "pipe() failed", 0, false };
        if (!opts.merge_stderr) {
            if (pipe(stderr_pipe) != 0) {
                close(stdout_pipe[0]); close(stdout_pipe[1]);
                return { -1, "", "pipe() failed", 0, false };
            }
        }
    }

    std::vector<const char*> argv;
    argv.push_back(abs_exec.c_str());
    for (const auto& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);

    pid_t pid = ::fork();
    if (pid < 0) {
        result.exit_code   = -1;
        result.stderr_out = "fork() failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // ── Child process ──────────────────────────────────────────
        ::setpgid(0, 0);

        // Security: Scrub dangerous environment variables
        static const char* const kDangerousEnv[] = {
            "LD_PRELOAD", "LD_LIBRARY_PATH", "PYTHONPATH", "RUBYLIB", "PERL5LIB",
            "DYLD_INSERT_LIBRARIES", "DYLD_LIBRARY_PATH", "HTTP_PROXY", "HTTPS_PROXY",
            nullptr
        };
        for (auto p = kDangerousEnv; *p; ++p) unsetenv(*p);

        if (opts.capture_output) {
            dup2(stdout_pipe[1], STDOUT_FILENO);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            if (!opts.merge_stderr) {
                dup2(stderr_pipe[1], STDERR_FILENO);
                close(stderr_pipe[0]); close(stderr_pipe[1]);
            } else {
                dup2(STDOUT_FILENO, STDERR_FILENO);
            }
        }

        if (!opts.work_dir.empty()) {
            if (chdir(opts.work_dir.c_str()) != 0) _exit(126);
        }

        for (const auto& [k, v] : opts.extra_env) {
            setenv(k.c_str(), v.c_str(), 1);
        }

        execvp(abs_exec.c_str(), const_cast<char* const*>(argv.data()));
        _exit(127);
    }

    // ── Parent process ──────────────────────────────────────────────
    if (opts.capture_output) {
        close(stdout_pipe[1]);
        if (!opts.merge_stderr) close(stderr_pipe[1]);
    }

    std::atomic<bool> timed_out{false};
    std::thread watchdog;
    if (opts.timeout_sec > 0) {
        watchdog = std::thread([&, pgid = pid]() {
            std::this_thread::sleep_for(std::chrono::seconds(opts.timeout_sec));
            if (!timed_out.exchange(true)) {
                ::kill(-pgid, SIGTERM);
                std::this_thread::sleep_for(
                    std::chrono::seconds(opts.kill_grace_sec));
                ::kill(-pgid, SIGKILL);
            }
        });
    }

    if (opts.capture_output) {
        char buf[4096];
        struct pollfd fds[2];
        int nfds = 0;
        if (stdout_pipe[0] >= 0) {
            fds[nfds].fd = stdout_pipe[0]; fds[nfds].events = POLLIN; ++nfds;
        }
        if (stderr_pipe[0] >= 0) {
            fds[nfds].fd = stderr_pipe[0]; fds[nfds].events = POLLIN; ++nfds;
        }

        while (nfds > 0) {
            int ret = ::poll(fds, nfds, 200);
            if (ret < 0 && errno == EINTR) continue;
            if (ret == 0 && timed_out.load()) break;

            bool any_open = false;
            for (int i = 0; i < nfds; ++i) {
                if (fds[i].fd < 0) continue;
                if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                    ssize_t n = ::read(fds[i].fd, buf, sizeof(buf));
                    if (n > 0) {
                        any_open = true;
                        if (fds[i].fd == stdout_pipe[0])
                            result.stdout_out.append(buf, n);
                        else
                            result.stderr_out.append(buf, n);
                    } else {
                        close(fds[i].fd);
                        fds[i].fd = -1;
                    }
                } else {
                    any_open = true;
                }
            }
            
            // Clean up fds array
            int j = 0;
            for (int i = 0; i < nfds; ++i) {
                if (fds[i].fd >= 0) fds[j++] = fds[i];
            }
            nfds = j;
            if (!any_open && nfds == 0) break;
        }
        if (stdout_pipe[0] >= 0) close(stdout_pipe[0]);
        if (stderr_pipe[0] >= 0) close(stderr_pipe[0]);
    }

    int status = 0;
    ::waitpid(pid, &status, 0);

    if (watchdog.joinable()) {
        timed_out.store(true);
        watchdog.join();
    }

    auto t1 = std::chrono::steady_clock::now();
    result.duration_sec = std::chrono::duration<double>(t1 - t0).count();
    result.timed_out    = timed_out.load();

    if (WIFEXITED(status))
        result.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        result.exit_code = 128 + WTERMSIG(status);

    return result;
}

int ExecEngine::run(const std::string& executable,
                     const std::vector<std::string>& args,
                     const Options& opts)
{
    Options o = opts;
    o.capture_output = false;
    auto r = do_exec(executable, args, o);
    return r.exit_code;
}

ExecResult ExecEngine::capture(const std::string& executable,
                               const std::vector<std::string>& args,
                               const Options& opts)
{
    return do_exec(executable, args, opts);
}

std::string ExecEngine::capture_line(const std::string& executable,
                                     const std::vector<std::string>& args,
                                     const Options& opts)
{
    auto r = do_exec(executable, args, opts);
    auto first_newline = r.stdout_out.find('\n');
    if (first_newline != std::string::npos)
        return r.stdout_out.substr(0, first_newline);
    return r.stdout_out;
}

} // namespace sspm
