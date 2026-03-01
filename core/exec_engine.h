#pragma once
// ═══════════════════════════════════════════════════════════
//  core/exec_engine.h — 统一安全执行层
//  直接取自 v1.9 exec_engine.h（精华，原样保留）
//  ╔═══════════════════════════════════════════════════════╗
//  ║  所有外部命令必须通过此层执行                           ║
//  ║  严禁直接调用 system() / popen() / execl with sh -c   ║
//  ║  参数必须 vector<string>，禁止字符串拼接执行            ║
//  ║  包名必须通过 InputValidator::pkg() 验证               ║
//  ╚═══════════════════════════════════════════════════════╝
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <atomic>
#include <thread>
#include <stdexcept>

namespace SSPM {

// ── 输入验证器 ────────────────────────────────────────────
class InputValidator {
public:
    struct Result {
        bool        ok     = false;
        std::string value;
        std::string reason;
        explicit operator bool() const { return ok; }
    };

    static Result pkg(const std::string& name);
    static Result pkgList(const std::vector<std::string>& names);
    static Result layerName(const std::string& name);
    static Result filePath(const std::string& path,
                           bool mustBeAbsolute = false,
                           bool mustExist      = false);
    static Result version(const std::string& ver);
    static Result executablePath(const std::string& path);
    static Result safeString(const std::string& s, size_t maxLen = 4096);

    static bool hasShellMetachars(const std::string& s);
};

// ── 执行结果 ──────────────────────────────────────────────
struct ExecResult {
    int         exitCode    = 0;
    std::string stdout_out;
    std::string stderr_out;
    double      durationSec = 0.0;
    bool        timedOut    = false;

    bool ok() const { return exitCode == 0 && !timedOut; }
    std::string diagnose() const;
};

// ── 执行选项 ──────────────────────────────────────────────
struct ExecOptions {
    bool captureOutput = true;
    bool mergeStderr   = false;
    int  timeoutSec    = 60;
    int  killGraceSec  = 3;
    std::string workDir;
    std::map<std::string, std::string> extraEnv;
};

// ── 执行引擎 ──────────────────────────────────────────────
class ExecEngine {
public:
    using Options = ExecOptions;

    // 执行命令，直接透传 stdout/stderr 到终端（安装时使用）
    static int run(const std::string& executable,
                   const std::vector<std::string>& args,
                   const Options& opts = {});

    // 执行命令并捕获输出
    static ExecResult capture(const std::string& executable,
                               const std::vector<std::string>& args,
                               const Options& opts = {});

    // 捕获第一行输出
    static std::string captureLine(const std::string& executable,
                                    const std::vector<std::string>& args,
                                    const Options& opts = {});

    // 检测命令是否存在（通过 access(X_OK)，不走 shell）
    static bool exists(const std::string& executable);

    // 安全 ping（内部用于网络检测）
    static bool runPing(const std::string& host,
                        int count      = 1,
                        int timeoutSec = 3);

    // 安全 mount/umount（取自 v1，Phase 2 隔离层用）
    static bool runMount(const std::string& fstype,
                         const std::string& device,
                         const std::string& mountpoint,
                         const std::vector<std::string>& options = {});
    static bool runUmount(const std::string& mountpoint, bool lazy = false);

    // 安全 curl（取自 v1，Amber PM / repo 下载用）
    static ExecResult runCurl(const std::string& url,
                               int  timeoutSec      = 30,
                               bool followRedirects = true);

private:
    static const char* const kAllowedPrefixes[];

    static std::string findInPath(const std::string& name);
    static void        auditLog(const std::string& executable,
                                const std::vector<std::string>& args);
    static ExecResult  doExec(const std::string& executable,
                               const std::vector<std::string>& args,
                               const Options& opts);
};

// ── 插件权限（预留，Phase 3 用）─────────────────────────
enum class PluginPerm : uint32_t {
    None           = 0,
    Filesystem     = 1 << 0,
    Network        = 1 << 1,
    PackageInstall = 1 << 2,
    ShellExec      = 1 << 3,
    NotifyUser     = 1 << 4,
    ReadConfig     = 1 << 5,
    WriteConfig    = 1 << 6,
    ReadDatabase   = 1 << 7,
    WriteDatabase  = 1 << 8,
};
inline PluginPerm operator|(PluginPerm a, PluginPerm b) {
    return static_cast<PluginPerm>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool hasPermission(PluginPerm granted, PluginPerm needed) {
    return (static_cast<uint32_t>(granted) &
            static_cast<uint32_t>(needed)) != 0;
}

const char* permName(PluginPerm p);
PluginPerm  parsePermissions(const std::vector<std::string>& perms);

class PluginPermissionChecker {
public:
    PluginPermissionChecker(const std::string& pluginName, PluginPerm granted);
    bool check(PluginPerm needed, const std::string& op = "") const;
    void enforce(PluginPerm needed, const std::string& op = "") const;
    bool checkShellExec(const std::string& cmdPreview) const;
private:
    std::string name_;
    PluginPerm  granted_;
    void logViolation(PluginPerm needed, const std::string& op) const;
};

} // namespace SSPM
// append these to ExecEngine class - added for v1 compat
