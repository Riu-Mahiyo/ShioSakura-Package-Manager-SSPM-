#include "sspm/backend.h"
#include <cstdlib>

namespace sspm::backends {

class nixBackend : public Backend {
public:
    std::string name() const override { return "nix"; }
    bool is_available() const override {
        return std::system("which nix > /dev/null 2>&1") == 0;
    }
    Result install(const Package& pkg) override {
        int r = std::system(("nix install " + pkg.name).c_str());
        return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"nix install failed":"" };
    }
    Result remove(const Package& pkg) override {
        int r = std::system(("nix remove " + pkg.name).c_str());
        return { r==0, "", r!=0?"nix remove failed":"" };
    }
    SearchResult search(const std::string& q) override {
        std::system(("nix search " + q).c_str()); return { {}, true, "" };
    }
    Result update() override {
        int r = std::system("nix update"); return { r==0, "", "" };
    }
    Result upgrade() override {
        int r = std::system("nix upgrade"); return { r==0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }
};

} // namespace sspm::backends
