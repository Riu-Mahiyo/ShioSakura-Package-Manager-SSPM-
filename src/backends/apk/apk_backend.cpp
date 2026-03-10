#include "sspm/backends/apk_backend.h"
#include "sspm/sspm-sdk.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <map>
#include <array>
#include <optional>

namespace sspm::backends {

// implementation details live in previous edit area; we simply move the
// definitions here so the header remains authoritative.  For brevity we
// reimplement certain helpers that were private in the class.

// static member initialization
ApkBackend::CmdFn ApkBackend::run_cmd = [](const std::string& c){ return std::system(c.c_str()); };
ApkBackend::ExecFn ApkBackend::exec_fn;


// helper used by a few methods
static std::optional<sspm::Package> parse_pkg_line(const std::string& line) {
    auto pos = line.find_last_of('-');
    if (pos == std::string::npos) return std::nullopt;
    sspm::Package pkg;
    pkg.name = line.substr(0, pos);
    pkg.version = line.substr(pos + 1);
    return pkg;
}

static std::pair<int,std::string> exec_capture(const std::string& cmd) {
    if (ApkBackend::exec_fn) {
        return ApkBackend::exec_fn(cmd);
    }
    std::array<char,128> buffer;
    std::string result;
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) return {-1, ""};
    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
    }
    int rc = pclose(pipe);
    if (WIFEXITED(rc)) rc = WEXITSTATUS(rc);
    return {rc, result};
}

// Now provide the methods from the header

std::string ApkBackend::id() const { return "backend.apk"; }
std::string ApkBackend::name() const { return "apk"; }
sspm::Version ApkBackend::version() const { return {1,1,0}; }

void ApkBackend::set_cmd_fn(CmdFn fn) { run_cmd = std::move(fn); }

void ApkBackend::set_exec_fn(ExecFn fn) { exec_fn = std::move(fn); }

bool ApkBackend::initialize(const std::map<std::string, std::string>&) {
    if (!is_available()) {
        SSPM_ERROR("apk backend initialization failed: backend not available on this platform");
        return false;
    }
    return true;
}

void ApkBackend::shutdown() {}

bool ApkBackend::is_available() const {
#ifdef SSPM_LINUX
    if (!sspm::BackendRegistry::is_alpine()) return false;
    return run_cmd("which apk > /dev/null 2>&1") == 0;
#else
    return false;
#endif
}

sspm::Result ApkBackend::install(const sspm::Package& pkg) {
    if (!is_available()) {
        return {false, "", "apk is not supported on this system"};
    }
    std::string cmd = "apk add " + pkg.name;
    int r = run_cmd(cmd);
    return { r == 0,
             r == 0 ? "Successfully installed " + pkg.name : "",
             r != 0 ? "apk add failed" : "" };
}

sspm::Result ApkBackend::remove(const sspm::Package& pkg) {
    if (!is_available()) {
        return {false, "", "apk is not supported on this system"};
    }
    std::string cmd = "apk del " + pkg.name;
    int r = run_cmd(cmd);
    return { r == 0,
             r == 0 ? "Successfully removed " + pkg.name : "",
             r != 0 ? "apk del failed" : "" };
}

sspm::SearchResult ApkBackend::search(const std::string& query) {
    if (!is_available()) {
        return {{}, false, "apk is not supported on this system"};
    }
    auto [rc, out] = exec_capture("apk search -v " + query);
    sspm::SearchResult res;
    res.success = (rc == 0);
    if (rc != 0) {
        res.error = "apk search failed";
        return res;
    }
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (auto pkg = parse_pkg_line(line)) res.packages.push_back(*pkg);
    }
    return res;
}

sspm::Result ApkBackend::update() {
    if (!is_available()) {
        return {false, "", "apk is not supported on this system"};
    }
    int r = run_cmd("apk update");
    return { r == 0, "Repository indices updated", r != 0 ? "apk update failed" : "" };
}

sspm::Result ApkBackend::upgrade() {
    if (!is_available()) {
        return {false, "", "apk is not supported on this system"};
    }
    int r = run_cmd("apk upgrade");
    return { r == 0, "System upgraded", r != 0 ? "apk upgrade failed" : "" };
}

sspm::PackageList ApkBackend::list_installed() {
    if (!is_available()) return {};
    auto [rc, out] = exec_capture("apk info -v");
    sspm::PackageList list;
    if (rc != 0) return list;
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line)) {
        if (auto p = parse_pkg_line(line)) list.push_back(*p);
    }
    return list;
}

std::optional<sspm::Package> ApkBackend::info(const std::string& name) {
    if (!is_available()) return std::nullopt;
    auto [rc, out] = exec_capture("apk info -v " + name);
    if (rc != 0) return std::nullopt;
    std::istringstream iss(out);
    std::string line;
    if (std::getline(iss, line)) {
        return parse_pkg_line(line);
    }
    return std::nullopt;
}

// registration macro still works as before
SSPM_REGISTER_BACKEND(ApkBackend)

} // namespace sspm::backends
