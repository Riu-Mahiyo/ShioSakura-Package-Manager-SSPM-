// ════════════════════════════════════════════════════════════
//  exec_engine.cpp — 统一安全执行层
//
//  ╔══════════════════════════════════════════════════════╗
//  ║  THIS IS THE ONLY FILE ALLOWED TO CALL fork/execvp  ║
//  ║  (除了 process.cpp 中的 SubprocessRunner)             ║
//  ║  所有其他代码必须通过 ExecEngine:: 接口               ║
//  ╚══════════════════════════════════════════════════════╝
// ════════════════════════════════════════════════════════════
#include "exec_engine.h"
#include "logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cassert>

// POSIX
#include <unistd.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

namespace SSPM {
namespace fs = std::filesystem;

// ════════════════════════════════════════════════════════════
//  InputValidator
// ════════════════════════════════════════════════════════════

// shell 元字符黑名单（严格）
static const char kShellMetachars[] =
    " \t\n\r;|&$`\\\"'<>(){}[]!#~*?^=%@";

bool InputValidator::hasShellMetachars(const std::string& s) {
    for (char c : s) {
        for (const char* p = kShellMetachars; *p; ++p) {
            if (c == *p) return true;
        }
    }
    return false;
}

InputValidator::Result InputValidator::pkg(const std::string& name) {
    if (name.empty())
        return {false, "", "包名不能为空"};

    if (name.size() > 255)
        return {false, "", "包名超长（>255字符）: " + name.substr(0, 32) + "..."};

    // 路径遍历检测
    if (name.find("..") != std::string::npos)
        return {false, "", "包名含路径遍历: " + name};

    // 白名单：[a-zA-Z0-9._+-]
    for (char c : name) {
        if (!isalnum(c) && c != '.' && c != '_' && c != '+' && c != '-') {
            char buf[64];
            snprintf(buf, sizeof(buf),
                "包名含非法字符 '\\x%02X'（%c）: %s",
                (unsigned char)c, isprint(c) ? c : '?', name.c_str());
            return {false, "", buf};
        }
    }

    // 不允许以 - 开头（防止被解析为命令行参数）
    if (name[0] == '-')
        return {false, "", "包名不能以 '-' 开头: " + name};

    return {true, name, ""};
}

InputValidator::Result InputValidator::pkgList(const std::vector<std::string>& names) {
    if (names.empty())
        return {false, "", "包名列表为空"};
    for (const auto& n : names) {
        auto r = pkg(n);
        if (!r) return r;
    }
    return {true, "", ""};
}

InputValidator::Result InputValidator::layerName(const std::string& name) {
    if (name.empty())
        return {false, "", "Layer 名称不能为空"};
    if (name.size() > 64)
        return {false, "", "Layer 名称超长"};
    for (char c : name) {
        if (!isalnum(c) && c != '.' && c != '_' && c != '-')
            return {false, "", "Layer 名含非法字符: " + name};
    }
    if (name[0] == '-' || name[0] == '.')
        return {false, "", "Layer 名称不能以 '-' 或 '.' 开头"};
    return {true, name, ""};
}

InputValidator::Result InputValidator::filePath(
    const std::string& path, bool mustBeAbsolute, bool mustExist)
{
    if (path.empty())
        return {false, "", "路径不能为空"};
    if (path.find('\0') != std::string::npos)
        return {false, "", "路径含 null 字节"};

    // 路径遍历检测（规范化后检查）
    try {
        fs::path p(path);
        // 检查每个分量
        for (const auto& comp : p) {
            if (comp == "..") {
                return {false, "", "路径含路径遍历 '..': " + path};
            }
        }
    } catch (...) {
        return {false, "", "路径解析失败: " + path};
    }

    if (mustBeAbsolute && path[0] != '/')
        return {false, "", "必须为绝对路径: " + path};

    if (mustExist && !fs::exists(path))
        return {false, "", "路径不存在: " + path};

    return {true, path, ""};
}

InputValidator::Result InputValidator::version(const std::string& ver) {
    if (ver.empty())
        return {false, "", "版本号不能为空"};
    if (ver.size() > 128)
        return {false, "", "版本号超长"};
    // 允许: [a-zA-Z0-9._+~:-]
    for (char c : ver) {
        if (!isalnum(c) && c != '.' && c != '_' && c != '+' &&
            c != '~' && c != ':' && c != '-') {
            return {false, "", "版本号含非法字符: " + ver};
        }
    }
    return {true, ver, ""};
}

InputValidator::Result InputValidator::executablePath(const std::string& path) {
    if (path.empty() || path[0] != '/')
        return {false, "", "可执行路径必须为绝对路径: " + path};

    // 白名单前缀
    static const char* const kSafeExecPrefixes[] = {
        "/usr/bin/", "/usr/sbin/", "/bin/", "/sbin/",
        "/usr/local/bin/", "/usr/local/sbin/",
        "/opt/homebrew/bin/",
        "/nix/store/",       // Nix 包路径
        "/run/current-system/sw/bin/",
        nullptr
    };
    bool inPrefix = false;
    for (auto p = kSafeExecPrefixes; *p; ++p) {
        if (path.rfind(*p, 0) == 0) { inPrefix = true; break; }
    }
    if (!inPrefix)
        return {false, "", "可执行路径不在安全前缀内: " + path};

    // 路径遍历
    if (path.find("..") != std::string::npos)
        return {false, "", "可执行路径含路径遍历: " + path};

    return {true, path, ""};
}

InputValidator::Result InputValidator::safeString(const std::string& s, size_t maxLen) {
    if (s.size() > maxLen)
        return {false, "", "字符串超长"};
    for (char c : s) {
        if ((unsigned char)c < 0x20 && c != '\t' && c != '\n')
            return {false, "", "字符串含控制字符"};
        if (c == '\0')
            return {false, "", "字符串含 null 字节"};
    }
    return {true, s, ""};
}

// ════════════════════════════════════════════════════════════
//  ExecResult
// ════════════════════════════════════════════════════════════
std::string ExecResult::diagnose() const {
    if (timedOut) return "超时";
    if (exitCode == 127) return "命令不存在（exit 127）";
    if (exitCode != 0)
        return "exit " + std::to_string(exitCode) +
               (stderr_out.empty() ? "" : ": " + stderr_out.substr(0, 200));
    return "成功";
}

// ════════════════════════════════════════════════════════════
//  ExecEngine — 内部实现
// ════════════════════════════════════════════════════════════
const char* const ExecEngine::kAllowedPrefixes[] = {
    "/usr/bin/", "/usr/sbin/", "/bin/", "/sbin/",
    "/usr/local/bin/", "/usr/local/sbin/",
    "/opt/homebrew/bin/",
    "/nix/store/",
    "/run/current-system/sw/bin/",
    nullptr
};

void ExecEngine::auditLog(const std::string& executable,
                           const std::vector<std::string>& args)
{
    // 构建可读的命令字符串（仅用于日志，不用于执行！）
    std::string cmd = "[ExecEngine] " + executable;
    for (const auto& a : args) cmd += " " + a;
    Logger::debug(cmd);
}

std::string ExecEngine::findInPath(const std::string& name) {
    // 如果已是绝对路径，直接返回
    if (!name.empty() && name[0] == '/') return name;

    const char* pathEnv = getenv("PATH");
    if (!pathEnv) pathEnv = "/usr/bin:/bin:/usr/sbin:/sbin";
    std::string pathStr(pathEnv);

    std::istringstream ss(pathStr);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        std::string full = dir + "/" + name;
        if (access(full.c_str(), X_OK) == 0) return full;
    }
    return "";
}

ExecResult ExecEngine::doExec(const std::string& executable,
                               const std::vector<std::string>& args,
                               const Options& opts)
{
    ExecResult result;
    auto t0 = std::chrono::steady_clock::now();

    // 1. 找到绝对路径
    std::string absExec = findInPath(executable);
    if (absExec.empty()) {
        result.exitCode   = 127;
        result.stderr_out = "命令不存在: " + executable;
        Logger::error("[ExecEngine] 命令不存在: " + executable);
        return result;
    }

    auditLog(absExec, args);

    // 2. 创建管道
    int stdoutPipe[2] = {-1, -1};
    int stderrPipe[2] = {-1, -1};
    if (opts.captureOutput) {
        pipe(stdoutPipe);
        if (!opts.mergeStderr) pipe(stderrPipe);
    }

    // 3. 构建 argv（零拷贝，生命周期在 execvp 之前）
    std::vector<const char*> argv;
    argv.push_back(absExec.c_str());
    for (const auto& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);

    // 4. fork
    pid_t pid = ::fork();
    if (pid < 0) {
        result.exitCode   = -1;
        result.stderr_out = "fork 失败: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // ── 子进程 ──────────────────────────────────────────
        // setpgid 确保 kill(-pgid) 可以杀净子树
        ::setpgid(0, 0);

        if (opts.captureOutput) {
            dup2(stdoutPipe[1], STDOUT_FILENO);
            close(stdoutPipe[0]); close(stdoutPipe[1]);
            if (!opts.mergeStderr) {
                dup2(stderrPipe[1], STDERR_FILENO);
                close(stderrPipe[0]); close(stderrPipe[1]);
            } else {
                dup2(STDOUT_FILENO, STDERR_FILENO);
            }
        }

        // 工作目录
        if (!opts.workDir.empty()) {
            if (chdir(opts.workDir.c_str()) != 0) _exit(126);
        }

        // 追加环境变量
        for (const auto& [k, v] : opts.extraEnv) {
            setenv(k.c_str(), v.c_str(), 1);
        }

        // ╔══════════════════════════════════════╗
        // ║  ONLY execvp HERE — NEVER shell -c   ║
        // ╚══════════════════════════════════════╝
        execvp(absExec.c_str(), const_cast<char* const*>(argv.data()));

        // execvp 失败
        fprintf(stderr, "[ExecEngine] execvp 失败: %s: %s\n",
                absExec.c_str(), strerror(errno));
        _exit(127);
    }

    // ── 父进程 ──────────────────────────────────────────────
    if (opts.captureOutput) {
        close(stdoutPipe[1]);
        if (!opts.mergeStderr) close(stderrPipe[1]);
    }

    // 超时监控线程
    std::atomic<bool> timedOut{false};
    std::thread watchdog;
    if (opts.timeoutSec > 0) {
        watchdog = std::thread([&, pgid = pid]() {
            std::this_thread::sleep_for(std::chrono::seconds(opts.timeoutSec));
            if (!timedOut.exchange(true)) {
                ::kill(-pgid, SIGTERM);
                std::this_thread::sleep_for(
                    std::chrono::seconds(opts.killGraceSec));
                ::kill(-pgid, SIGKILL);
            }
        });
    }

    // 读取输出
    if (opts.captureOutput) {
        char buf[4096];
        struct pollfd fds[2];
        int nfds = 0;
        if (stdoutPipe[0] >= 0) {
            fds[nfds].fd = stdoutPipe[0]; fds[nfds].events = POLLIN; ++nfds;
        }
        if (stderrPipe[0] >= 0) {
            fds[nfds].fd = stderrPipe[0]; fds[nfds].events = POLLIN; ++nfds;
        }

        while (true) {
            int ret = ::poll(fds, nfds, 200);
            if (ret < 0 && errno == EINTR) continue;
            if (ret == 0 && timedOut.load()) break;

            bool anyOpen = false;
            for (int i = 0; i < nfds; ++i) {
                if (fds[i].fd < 0) continue;
                if (!(fds[i].revents & (POLLIN | POLLHUP))) { anyOpen = true; continue; }
                ssize_t n = ::read(fds[i].fd, buf, sizeof(buf));
                if (n <= 0) { close(fds[i].fd); fds[i].fd = -1; continue; }
                anyOpen = true;
                std::string chunk(buf, n);
                if (fds[i].fd == stdoutPipe[0])
                    result.stdout_out += chunk;
                else
                    result.stderr_out += chunk;
            }
            if (!anyOpen) break;
        }
        if (stdoutPipe[0] >= 0) close(stdoutPipe[0]);
        if (stderrPipe[0] >= 0) close(stderrPipe[0]);
    }

    // 等待子进程
    int status = 0;
    ::waitpid(pid, &status, 0);

    // 取消超时监控
    if (watchdog.joinable()) {
        timedOut.store(true);
        watchdog.join();
    }

    auto t1 = std::chrono::steady_clock::now();
    result.durationSec = std::chrono::duration<double>(t1 - t0).count();
    result.timedOut    = timedOut.load();

    if (WIFEXITED(status))
        result.exitCode = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        result.exitCode = 128 + WTERMSIG(status);

    return result;
}

// ── 公开接口 ───────────────────────────────────────────────
int ExecEngine::run(const std::string& executable,
                     const std::vector<std::string>& args,
                     const Options& opts)
{
    Options o = opts;
    o.captureOutput = false;
    auto r = doExec(executable, args, o);
    return r.exitCode;
}

ExecResult ExecEngine::capture(const std::string& executable,
                                 const std::vector<std::string>& args,
                                 const Options& opts)
{
    Options o = opts;
    o.captureOutput = true;
    return doExec(executable, args, o);
}

std::string ExecEngine::captureLine(const std::string& executable,
                                     const std::vector<std::string>& args,
                                     const Options& opts)
{
    auto r = capture(executable, args, opts);
    if (!r.ok()) return "";
    auto& s = r.stdout_out;
    auto pos = s.find('\n');
    return pos == std::string::npos ? s : s.substr(0, pos);
}

bool ExecEngine::exists(const std::string& executable) {
    // 通过 access(X_OK) 检测，不走 shell
    return !findInPath(executable).empty();
}

bool ExecEngine::runMount(const std::string& fstype,
                           const std::string& device,
                           const std::string& mountpoint,
                           const std::vector<std::string>& options)
{
    // 严格校验每个参数
    auto checkPath = [](const std::string& p, const char* field) -> bool {
        if (p.find("..") != std::string::npos || p.find('\0') != std::string::npos) {
            Logger::error(std::string("[ExecEngine::mount] ") + field + " 含非法字符: " + p);
            return false;
        }
        return true;
    };

    if (!checkPath(device, "device") || !checkPath(mountpoint, "mountpoint"))
        return false;

    // 验证 options 中无 shell 元字符
    for (const auto& opt : options) {
        if (InputValidator::hasShellMetachars(opt)) {
            Logger::error("[ExecEngine::mount] mount option 含非法字符: " + opt);
            return false;
        }
    }

    std::vector<std::string> args = {"-t", fstype};
    if (!options.empty()) {
        std::string optStr;
        for (size_t i = 0; i < options.size(); ++i) {
            if (i) optStr += ",";
            optStr += options[i];
        }
        args.push_back("-o");
        args.push_back(optStr);
    }
    args.push_back(device);
    args.push_back(mountpoint);

    Options o;
    o.timeoutSec = 10;
    int ret = run("/bin/mount", args, o);
    if (ret == 0) {
        Logger::success("[mount] " + device + " → " + mountpoint);
    } else {
        Logger::error("[mount] 失败 exit=" + std::to_string(ret));
    }
    return ret == 0;
}

bool ExecEngine::runUmount(const std::string& mountpoint, bool lazy) {
    auto r = InputValidator::filePath(mountpoint, true);
    if (!r) {
        Logger::error("[ExecEngine::umount] 非法路径: " + r.reason);
        return false;
    }
    std::vector<std::string> args;
    if (lazy) args.push_back("-l");
    args.push_back(mountpoint);
    Options o; o.timeoutSec = 10;
    return run("/bin/umount", args, o) == 0;
}

bool ExecEngine::runPing(const std::string& host, int count, int timeoutSec) {
    // 严格校验 host（只允许主机名/IP 字符）
    for (char c : host) {
        if (!isalnum(c) && c != '.' && c != '-' && c != ':') {
            Logger::error("[ExecEngine::ping] 非法 host: " + host);
            return false;
        }
    }
    // 使用 execvp，参数独立，无 shell 注入风险
    Options o; o.timeoutSec = timeoutSec + 2; o.captureOutput = true;
    auto r = capture("/bin/ping", {
        "-c", std::to_string(count),
        "-W", std::to_string(std::max(1, timeoutSec)),
        host
    }, o);
    return r.exitCode == 0;
}

ExecResult ExecEngine::runCurl(const std::string& url,
                                 int timeoutSec,
                                 bool followRedirects)
{
    // URL 基本安全检查
    if (url.find('\0') != std::string::npos || url.find('"') != std::string::npos ||
        url.find('\'') != std::string::npos || url.find('`') != std::string::npos ||
        url.find(';') != std::string::npos)
    {
        ExecResult r;
        r.exitCode = -1;
        r.stderr_out = "[ExecEngine::curl] URL 含非法字符";
        Logger::error(r.stderr_out);
        return r;
    }

    // 必须是 http:// 或 https://
    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
        ExecResult r;
        r.exitCode = -1;
        r.stderr_out = "[ExecEngine::curl] 只允许 http/https URL";
        Logger::error(r.stderr_out);
        return r;
    }

    std::vector<std::string> args = {
        "--no-progress-meter",
        "--fail",
        "--max-time", std::to_string(timeoutSec),
        "--connect-timeout", "10",
        "-A", "sky-package-manager/1.8",
    };
    if (followRedirects) args.push_back("-L");
    // URL 最后传入（不能注入额外参数）
    args.push_back("--");
    args.push_back(url);

    Options o;
    o.timeoutSec    = timeoutSec + 5;
    o.captureOutput = true;
    return capture("curl", args, o);
}

// ════════════════════════════════════════════════════════════
//  PluginPermission
// ════════════════════════════════════════════════════════════
const char* permName(PluginPerm p) {
    switch (p) {
        case PluginPerm::Filesystem:     return "filesystem";
        case PluginPerm::Network:        return "network";
        case PluginPerm::PackageInstall: return "package_install";
        case PluginPerm::ShellExec:      return "shell_exec";
        case PluginPerm::NotifyUser:     return "notify_user";
        case PluginPerm::ReadConfig:     return "read_config";
        case PluginPerm::WriteConfig:    return "write_config";
        case PluginPerm::ReadDatabase:   return "read_database";
        case PluginPerm::WriteDatabase:  return "write_database";
        default:                         return "none";
    }
}

PluginPerm parsePermissions(const std::vector<std::string>& perms) {
    PluginPerm result = PluginPerm::None;
    for (const auto& s : perms) {
        if      (s == "filesystem")      result = result | PluginPerm::Filesystem;
        else if (s == "network")         result = result | PluginPerm::Network;
        else if (s == "package_install") result = result | PluginPerm::PackageInstall;
        else if (s == "shell_exec")      result = result | PluginPerm::ShellExec;
        else if (s == "notify_user")     result = result | PluginPerm::NotifyUser;
        else if (s == "read_config")     result = result | PluginPerm::ReadConfig;
        else if (s == "write_config")    result = result | PluginPerm::WriteConfig;
        else if (s == "read_database")   result = result | PluginPerm::ReadDatabase;
        else if (s == "write_database")  result = result | PluginPerm::WriteDatabase;
        else {
            Logger::warning("未知插件权限声明: " + s + "（已忽略）");
        }
    }
    return result;
}

// ════════════════════════════════════════════════════════════
//  PluginPermissionChecker
// ════════════════════════════════════════════════════════════
PluginPermissionChecker::PluginPermissionChecker(
    const std::string& pluginName, PluginPerm granted)
    : name_(pluginName), granted_(granted)
{}

void PluginPermissionChecker::logViolation(PluginPerm needed,
                                            const std::string& op) const
{
    std::string msg = "🚫 插件权限拒绝 [" + name_ + "] 需要权限: " +
                      permName(needed);
    if (!op.empty()) msg += "  操作: " + op;
    Logger::warning(msg);

    // 写入安全审计日志（独立文件）
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base = xdg ? std::string(xdg) + "/sky"
                           : std::string(getenv("HOME") ? getenv("HOME") : "/tmp") +
                             "/.local/share/sky";
    std::ofstream f(base + "/plugin-security.log", std::ios::app);
    if (f) {
        auto now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        char ts[32];
        std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        f << ts << " PERM_DENIED plugin=" << name_
          << " needed=" << permName(needed)
          << " op=" << op << "\n";
    }
}

bool PluginPermissionChecker::check(PluginPerm needed,
                                     const std::string& op) const
{
    if (!hasPermission(granted_, needed)) {
        logViolation(needed, op);
        return false;
    }
    return true;
}

void PluginPermissionChecker::enforce(PluginPerm needed,
                                       const std::string& op) const
{
    if (!check(needed, op)) {
        throw std::runtime_error(
            "插件 [" + name_ + "] 权限不足: 需要 " + permName(needed));
    }
}

bool PluginPermissionChecker::checkShellExec(const std::string& cmdPreview) const {
    // ShellExec 额外：无论是否有权限，都记录到审计日志
    const char* xdg = getenv("XDG_DATA_HOME");
    std::string base = xdg ? std::string(xdg) + "/sky"
                           : std::string(getenv("HOME") ? getenv("HOME") : "/tmp") +
                             "/.local/share/sky";
    std::ofstream f(base + "/plugin-security.log", std::ios::app);
    if (f) {
        auto now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        char ts[32];
        std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        bool allowed = hasPermission(granted_, PluginPerm::ShellExec);
        f << ts << " SHELL_EXEC_" << (allowed ? "ALLOWED" : "DENIED")
          << " plugin=" << name_
          << " cmd=" << cmdPreview.substr(0, 100) << "\n";
    }

    if (!hasPermission(granted_, PluginPerm::ShellExec)) {
        Logger::error("🚫 插件 [" + name_ + "] 无 shell_exec 权限（默认拒绝）");
        Logger::tip("  插件需要在 plugin.toml 中声明: permissions = [\"shell_exec\"]");
        Logger::tip("  但 shell_exec 为高危权限，需要用户确认授权");
        return false;
    }
    Logger::warning("⚠  插件 [" + name_ + "] 正在执行 shell 命令（已授权但需注意）");
    return true;
}

} // namespace SSPM
