// layer/backends/flatpak_backend.cpp — Flatpak Backend
#include "flatpak_backend.h"
#include "../../core/exec_engine.h"
#include "../../core/logger.h"

namespace SSPM {

bool FlatpakBackend::available() const {
    return ExecEngine::exists("flatpak");
}

bool FlatpakBackend::hasFlathub() {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    auto r = ExecEngine::capture("flatpak", {"remotes"}, opts);
    return r.ok() && r.stdout_out.find("flathub") != std::string::npos;
}

bool FlatpakBackend::addFlathub() {
    Logger::step("添加 Flathub remote...");
    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 30;
    int rc = ExecEngine::run("flatpak",
        {"remote-add", "--if-not-exists", "flathub",
         "https://dl.flathub.org/repo/flathub.flatpakrepo"}, opts);
    if (rc == 0) Logger::ok("Flathub 已添加");
    else Logger::error("添加 Flathub 失败");
    return rc == 0;
}

BackendResult FlatpakBackend::install(const std::vector<std::string>& pkgs) {
    // flatpak 的包名格式允许 . 号（如 org.gimp.GIMP），需要更宽松的验证
    for (auto& p : pkgs) {
        auto v = InputValidator::safeString(p, 128);
        if (!v) return {1, "[flatpak] 包名非法: " + v.reason};
    }

    if (!hasFlathub()) {
        Logger::warn("Flathub remote 未配置，尝试自动添加...");
        addFlathub();
    }

    Logger::step("flatpak install -y flathub " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"install", "-y", "--noninteractive", "flathub"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("flatpak", args, opts);
    if (rc == 0) Logger::ok("安装成功");
    else Logger::error("flatpak 返回 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult FlatpakBackend::remove(const std::vector<std::string>& pkgs) {
    for (auto& p : pkgs) {
        auto v = InputValidator::safeString(p, 128);
        if (!v) return {1, "[flatpak] 包名非法: " + v.reason};
    }

    Logger::step("flatpak uninstall -y " + [&]{
        std::string s; for (auto& p : pkgs) s += p + " "; return s; }());

    std::vector<std::string> args = {"uninstall", "-y"};
    args.insert(args.end(), pkgs.begin(), pkgs.end());

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 120;

    int rc = ExecEngine::run("flatpak", args, opts);
    if (rc == 0) Logger::ok("卸载成功");
    else Logger::error("flatpak uninstall 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult FlatpakBackend::upgrade() {
    Logger::step("flatpak update -y");

    ExecEngine::Options opts;
    opts.captureOutput = false;
    opts.timeoutSec    = 600;

    int rc = ExecEngine::run("flatpak", {"update", "-y"}, opts);
    if (rc == 0) Logger::ok("Flatpak 应用更新完成");
    else Logger::error("flatpak update 失败 exit=" + std::to_string(rc));
    return {rc, ""};
}

BackendResult FlatpakBackend::search(const std::string& query) {
    auto v = InputValidator::safeString(query, 128);
    if (!v) return {1, "查询字符串非法: " + v.reason};

    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 30;

    auto r = ExecEngine::capture("flatpak", {"search", "--", query}, opts);
    return {r.exitCode, r.stdout_out};
}

std::string FlatpakBackend::installedVersion(const std::string& pkg) {
    ExecEngine::Options opts;
    opts.captureOutput = true;
    opts.timeoutSec    = 10;
    // flatpak info --show-version <pkg>
    auto r = ExecEngine::capture("flatpak", {"info", "--show-version", "--", pkg}, opts);
    if (!r.ok()) return "";
    auto ver = r.stdout_out;
    while (!ver.empty() && (ver.back() == '\n' || ver.back() == '\r'))
        ver.pop_back();
    return ver;
}

} // namespace SSPM
