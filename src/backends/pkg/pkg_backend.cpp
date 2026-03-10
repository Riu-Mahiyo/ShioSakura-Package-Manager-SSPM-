#include "sspm/backend.h"
#include <cstdlib>

namespace sspm::backends {

class pkgBackend : public Backend {
public:
    std::string name() const override { return "pkg"; }
    bool is_available() const override {
        return std::system("which pkg > /dev/null 2>&1") == 0;
    }
    Result install(const Package& pkg) override {
        int r = std::system(("pkg install " + pkg.name).c_str());
        return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"pkg install failed":"" };
    }
    Result remove(const Package& pkg) override {
        int r = std::system(("pkg remove " + pkg.name).c_str());
        return { r==0, "", r!=0?"pkg remove failed":"" };
    }
    SearchResult search(const std::string& q) override {
        std::system(("pkg search " + q).c_str()); return { {}, true, "" };
    }
    Result update() override {
        int r = std::system("pkg update"); return { r==0, "", "" };
    }
    Result upgrade() override {
        int r = std::system("pkg upgrade"); return { r==0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }
};

} // namespace sspm::backends
