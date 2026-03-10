#pragma once

#include "sspm/backend.h"
#include <functional>
#include <optional>
#include <string>

namespace sspm {
namespace backends {

class ApkBackend : public sspm::Backend {
public:
    using CmdFn = std::function<int(const std::string&)>;

    // command executor used by the backend; tests can replace this to avoid
    // launching real `apk` processes.  default value is std::system.
    static CmdFn run_cmd;
    static void set_cmd_fn(CmdFn fn);

    // execution helper for capturing output; allows tests to feed fake stdout
    using ExecFn = std::function<std::pair<int,std::string>(const std::string&)>;
    static ExecFn exec_fn;
    static void set_exec_fn(ExecFn fn);

    // IModule interface
    std::string id() const override;
    std::string name() const override;
    sspm::Version version() const override;
    bool initialize(const std::map<std::string, std::string>& context) override;
    void shutdown() override;

    // Backend interface
    bool is_available() const override;
    sspm::Result install(const sspm::Package& pkg) override;
    sspm::Result remove(const sspm::Package& pkg) override;
    sspm::SearchResult search(const std::string& query) override;
    sspm::Result update() override;
    sspm::Result upgrade() override;
    sspm::PackageList list_installed() override;
    std::optional<sspm::Package> info(const std::string& name) override;
};

} // namespace backends
} // namespace sspm
