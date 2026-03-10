// ═══════════════════════════════════════════════════════════
//  cli/cli_router.cpp — CLI 命令路由实现
// ═══════════════════════════════════════════════════════════
#include "cli_router.h"
#include "../core/detect.h"
#include "../core/logger.h"
#include "../core/exec_engine.h"
#include "../layer/layer_manager.h"
#include "../layer/backends/amber_backend.h"
#include "../layer/backends/nix_backend.h"
#include "../layer/backends/brew_backend.h"
#include "../doctor/doctor.h"
#include "../db/skydb.h"
#include "../transaction/transaction.h"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>

#define SSPM_VERSION "3.0.0"

namespace SSPM {

// ── 解析 argv ────────────────────────────────────────────

int CliRouter::dispatch(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];
    std::vector<std::string> positional;
    std::string backendOverride;

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--backend=", 0) == 0) backendOverride = a.substr(10);
        else if (a == "--help" || a == "-h")  return cmdHelp();
        else if (a[0] != '-') positional.push_back(a);
    }

    // ── 主命令路由 ─────────────────────────────────────────
    if (cmd == "install" || cmd == "i") {
        if (positional.empty()) {
            Logger::error("install 需要至少一个包名");
            return 1;
        }
        return cmdInstall(positional, backendOverride);
    }
    if (cmd == "remove" || cmd == "rm" || cmd == "uninstall") {
        if (positional.empty()) { Logger::error("remove 需要包名"); return 1; }
        return cmdRemove(positional, backendOverride);
    }
    if (cmd == "upgrade" || cmd == "update") return cmdUpgrade(backendOverride);
    if (cmd == "search"  || cmd == "s") {
        if (positional.empty()) { Logger::error("search 需要查询词"); return 1; }
        return cmdSearch(positional[0], backendOverride);
    }
    if (cmd == "list"    || cmd == "ls")  return cmdList();
    if (cmd == "doctor")                  return cmdDoctor();
    if (cmd == "version" || cmd == "--version" || cmd == "-V") return cmdVersion();
    if (cmd == "help"    || cmd == "--help" || cmd == "-h")    return cmdHelp();

    // ── v2.3 新命令 ────────────────────────────────────────
    if (cmd == "backends")               return cmdBackends();
    if (cmd == "db") {
        std::string sub = positional.empty() ? "list" : positional[0];
        std::string pkgArg = positional.size() > 1 ? positional[1] : "";
        return cmdDb(sub, pkgArg);
    }
    if (cmd == "lock")                   return cmdLockStatus();
    if (cmd == "amber-token") {
        if (positional.empty()) { Logger::error("用法: sspm amber-token <token>"); return 1; }
        return cmdAmberToken(positional[0]);
    }
    if (cmd == "fix-path") {
        if (positional.empty() || positional[0] == "nix")  {
            NixBackend::fixPath();
        }
        if (positional.empty() || positional[0] == "brew") {
            BrewBackend::fixPath();
        }
        return 0;
    }

    Logger::error("未知命令: " + cmd);
    Logger::info("运行 sspm help 查看可用命令");
    return 1;
}


// ── 命令实现 ─────────────────────────────────────────────

int CliRouter::cmdInstall(const std::vector<std::string>& pkgs,
                           const std::string& backendName)
{
    // 1. 获取 backend
    LayerBackend* be = backendName.empty()
        ? LayerManager::getDefault()
        : LayerManager::get(backendName);

    if (!be) {
        Logger::error("找不到可用的 backend" +
            (backendName.empty() ? "" : ": " + backendName));
        Logger::info("运行 sspm doctor 查看系统状态");
        return 1;
    }

    // 2. 获取安装锁
    if (!Transaction::acquireLock()) return 1;

    // 3. 事务包装（含 rollback 函数）
    Transaction txn("install " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }(),
        TxPolicy::AtomicAll);
    txn.begin();

    // 注册每个包的 rollback = remove（安装成功后触发）
    for (auto& p : pkgs) {
        txn.addOp("install", be->name(), p, [be, p]() -> bool {
            return be->remove({p}).ok();
        });
    }

    // 4. 执行安装
    auto result = be->install(pkgs);

    // 5. 更新 SkyDB
    if (result.ok()) {
        for (size_t i = 0; i < pkgs.size(); ++i) {
            std::string ver = be->installedVersion(pkgs[i]);
            SkyDB::recordInstall(pkgs[i], be->name(), ver);
            txn.markDone(i);
        }
        txn.commit();
    } else {
        txn.abort("backend exit=" + std::to_string(result.exitCode));
    }

    Transaction::releaseLock();
    return result.ok() ? 0 : 1;
}

int CliRouter::cmdRemove(const std::vector<std::string>& pkgs,
                          const std::string& backendName)
{
    LayerBackend* be = backendName.empty()
        ? LayerManager::getDefault()
        : LayerManager::get(backendName);

    if (!be) {
        Logger::error("找不到可用的 backend");
        return 1;
    }

    if (!Transaction::acquireLock()) return 1;

    Transaction txn("remove " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());
    txn.begin();
    for (auto& p : pkgs) txn.addOp("remove", be->name(), p);

    auto result = be->remove(pkgs);

    if (result.ok()) {
        for (size_t i = 0; i < pkgs.size(); ++i) {
            SkyDB::recordRemove(pkgs[i]);
            txn.markDone(i);
        }
        txn.commit();
    } else {
        txn.abort("backend exit=" + std::to_string(result.exitCode));
    }

    Transaction::releaseLock();
    return result.ok() ? 0 : 1;
}

int CliRouter::cmdUpgrade(const std::string& backendName) {
    LayerBackend* be = backendName.empty()
        ? LayerManager::getDefault()
        : LayerManager::get(backendName);

    if (!be) {
        Logger::error("找不到可用的 backend");
        return 1;
    }

    if (!Transaction::acquireLock()) return 1;

    Transaction txn("upgrade via " + be->name());
    txn.begin();
    txn.addOp("upgrade", be->name(), "*");

    auto result = be->upgrade();

    if (result.ok()) {
        txn.markDone(0);
        txn.commit();
    } else {
        txn.abort("backend exit=" + std::to_string(result.exitCode));
    }

    Transaction::releaseLock();
    return result.ok() ? 0 : 1;
}

int CliRouter::cmdSearch(const std::string& query,
                          const std::string& backendName)
{
    LayerBackend* be = backendName.empty()
        ? LayerManager::getDefault()
        : LayerManager::get(backendName);

    if (!be) {
        Logger::error("找不到可用的 backend");
        return 1;
    }

    Logger::step("搜索: " + query + " (via " + be->name() + ")");
    auto result = be->search(query);
    if (!result.output.empty()) {
        std::cout << result.output;
    }
    return result.ok() ? 0 : 1;
}

int CliRouter::cmdList() {
    SkyDB::load();
    auto all = SkyDB::all();

    if (all.empty()) {
        std::cout << "       (SkyDB 为空，暂无已安装包记录)\n";
        return 0;
    }

    std::cout << "\n";
    std::cout << C_BOLD
              << std::left
              << std::setw(32) << "Package"
              << std::setw(20) << "Version"
              << std::setw(12) << "Backend"
              << C_RESET << "\n";
    std::cout << std::string(64, '-') << "\n";

    for (auto& [name, rec] : all) {
        std::cout << std::left
                  << std::setw(32) << name
                  << std::setw(20) << (rec.version.empty() ? "-" : rec.version)
                  << std::setw(12) << rec.backend
                  << "\n";
    }
    std::cout << "\n" << all.size() << " package(s) in SkyDB\n\n";
    return 0;
}

int CliRouter::cmdDoctor() {
    std::cout << C_BOLD "\nSky Doctor — 系统健康检查\n" C_RESET;
    std::cout << "OS: " << Detect::osName()
              << " (" << Detect::arch() << ")\n";

    auto issues = Doctor::runAll();
    Doctor::printReport(issues);
    return 0;
}

int CliRouter::cmdVersion() {
    std::cout << "SSPM " SSPM_VERSION "\n";
    std::cout << "OS: " << Detect::osName() << " / " << Detect::arch() << "\n";
    auto be = Detect::defaultBackend();
    std::cout << "Default backend: " << (be.empty() ? "(none)" : be) << "\n";
    return 0;
}

int CliRouter::cmdHelp() {
    printUsage();
    return 0;
}

void CliRouter::printUsage() {
    std::cout << C_BOLD "SSPM " SSPM_VERSION C_RESET " — 跨平台包管理器\n\n";
    std::cout << "用法: sky <command> [options] [packages...]\n\n";
    std::cout << C_BOLD "核心命令\n" C_RESET;
    std::cout << "  install <pkg...>     安装包\n";
    std::cout << "  remove  <pkg...>     卸载包\n";
    std::cout << "  upgrade              升级全部\n";
    std::cout << "  search  <query>      搜索包\n";
    std::cout << "  list                 列出 SkyDB 已记录包\n\n";
    std::cout << C_BOLD "系统命令\n" C_RESET;
    std::cout << "  doctor               系统健康检查（12项）\n";
    std::cout << "  backends             列出所有 backend 及可用性\n";
    std::cout << "  db [list|query|orphan]  SkyDB 数据库管理\n";
    std::cout << "  lock                 显示安装锁状态\n";
    std::cout << "  fix-path [nix|brew]  修复 PATH 配置\n\n";
    std::cout << C_BOLD "Amber PM\n" C_RESET;
    std::cout << "  amber-token <tok>    设置 Amber PM token\n\n";
    std::cout << C_BOLD "选项\n" C_RESET;
    std::cout << "  --backend=<n>  指定 backend (apt/pacman/dnf/brew/amber/...)\n\n";
    std::cout << C_BOLD "支持的 Backend\n" C_RESET;
    std::cout << "  Linux:  apt dpkg pacman aur dnf zypper portage nix flatpak snap portable spkg amber\n";
    std::cout << "  macOS:  brew macports npm pip spkg amber\n";
    std::cout << "  BSD:    pkg spkg amber\n";
    std::cout << "  通用:   npm pip wine\n\n";
    std::cout << "示例:\n";
    std::cout << "  sspm install curl\n";
    std::cout << "  sspm install gimp --backend=flatpak\n";
    std::cout << "  sspm backends\n";
    std::cout << "  sspm db orphan\n\n";
}


// ══════════════════════════════════════════════════════════
//  v2.3 新命令实现
// ══════════════════════════════════════════════════════════

int CliRouter::cmdBackends() {
    auto available = LayerManager::availableNames();
    auto all       = LayerManager::allNames();

    std::cout << C_BOLD "\n可用 Backend（当前系统已安装）\n" C_RESET;
    if (available.empty()) {
        Logger::warn("没有可用的 backend，请安装包管理器");
    } else {
        for (auto& b : available) {
            std::cout << "  " C_GREEN "✓" C_RESET "  " << b << "\n";
        }
    }

    // 显示不可用的
    std::cout << C_BOLD "\n所有支持的 Backend\n" C_RESET;
    std::set<std::string> avSet(available.begin(), available.end());
    for (auto& b : all) {
        bool avail = avSet.count(b) > 0;
        std::cout << "  " << (avail ? C_GREEN "✓" : C_GRAY "✗") << C_RESET
                  << "  " << b << "\n";
    }
    std::cout << "\n";

    auto defBe = Detect::defaultBackend();
    if (!defBe.empty()) {
        std::cout << "默认 backend: " C_CYAN << defBe << C_RESET "\n\n";
    }
    return 0;
}

int CliRouter::cmdDb(const std::string& sub, const std::string& pkg) {
    SkyDB::load();

    if (sub == "list" || sub.empty()) {
        auto all = SkyDB::all();
        if (all.empty()) {
            std::cout << "(SkyDB 为空)\n";
            return 0;
        }
        std::cout << C_BOLD;
        std::cout << std::left << std::setw(28) << "包名"
                  << std::setw(14) << "版本"
                  << std::setw(10) << "Backend"
                  << "安装时间\n" C_RESET;
        std::cout << std::string(72, '-') << "\n";
        for (auto& [name, rec] : all) {
            std::cout << std::left << std::setw(28) << name
                      << std::setw(14) << (rec.version.empty() ? "-" : rec.version)
                      << std::setw(10) << rec.backend
                      << rec.installTime << "\n";
        }
        std::cout << "\n共 " << all.size() << " 个记录\n";
        return 0;
    }

    if (sub == "query" || sub == "info") {
        if (pkg.empty()) { Logger::error("db query 需要包名"); return 1; }
        auto rec = SkyDB::query(pkg);
        if (!rec) { Logger::warn("未找到包: " + pkg); return 1; }
        std::cout << "名称:     " << rec->name     << "\n";
        std::cout << "版本:     " << rec->version   << "\n";
        std::cout << "Backend:  " << rec->backend   << "\n";
        std::cout << "安装时间: " << rec->installTime << "\n";
        return 0;
    }

    if (sub == "orphan") {
        // 孤包：SkyDB 中有记录但 backend 报告已卸载的包
        Logger::info("孤包检测（实验性）...");
        auto all = SkyDB::all();
        int orphans = 0;
        for (auto& [name, rec] : all) {
            auto be = LayerManager::get(rec.backend);
            if (!be) continue;
            auto ver = be->installedVersion(name);
            if (ver.empty()) {
                std::cout << C_YELLOW "[孤包] " C_RESET << name
                          << " (backend=" << rec.backend << ")\n";
                orphans++;
            }
        }
        if (orphans == 0) Logger::ok("没有发现孤包");
        else std::cout << "\n发现 " << orphans << " 个孤包（可运行 sspm remove 清理）\n";
        return 0;
    }

    Logger::error("未知 db 子命令: " + sub);
    Logger::info("可用: list, query <pkg>, orphan");
    return 1;
}

int CliRouter::cmdLockStatus() {
    std::string lockfile;
    const char* xdg = getenv("XDG_RUNTIME_DIR");
    if (xdg && *xdg) lockfile = std::string(xdg) + "/sspm.lock";
    else lockfile = "/tmp/sspm.lock";

    struct stat st;
    if (::stat(lockfile.c_str(), &st) != 0) {
        Logger::ok("没有活动的安装锁");
        return 0;
    }

    // 读取 PID
    int fd = ::open(lockfile.c_str(), O_RDONLY);
    char buf[32] = {};
    if (fd >= 0) { ::read(fd, buf, sizeof(buf)-1); ::close(fd); }
    std::string pid = buf[0] ? std::string(buf) : "unknown";

    // 检查进程是否仍存活
    if (buf[0]) {
        pid_t p = (pid_t)atoi(buf);
        if (::kill(p, 0) != 0) {
            Logger::warn("发现陈旧锁 (PID " + pid + " 已不存在)");
            Logger::info("运行 rm " + lockfile + " 清理");
            return 0;
        }
    }

    Logger::info("安装锁由 PID " + pid + " 持有");
    Logger::info("锁文件: " + lockfile);
    return 0;
}

int CliRouter::cmdAmberToken(const std::string& token) {
    // 简单格式验证
    if (token.size() < 8) {
        Logger::error("Token 格式无效（太短）");
        return 1;
    }
    if (!AmberBackend::setToken(token)) {
        Logger::error("保存 Token 失败");
        return 1;
    }
    Logger::ok("Amber PM token 已设置");
    return 0;
}

} // namespace SSPM

// ──────────────────────────────────────────────────────────
// 以下命令由 patch 追加（v2.3 gap 补齐）
// ──────────────────────────────────────────────────────────
