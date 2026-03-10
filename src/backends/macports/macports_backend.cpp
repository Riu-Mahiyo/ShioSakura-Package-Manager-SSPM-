#include "sspm/backend.h"
#include <cstdlib>

namespace sspm::backends {

class macportsBackend : public Backend {
public:
    std::string name() const override { return "macports"; }
    bool is_available() const override {
        return std::system("which macports > /dev/null 2>&1") == 0;
    }
    Result install(const Package& pkg) override {
        int r = std::system(("macports install " + pkg.name).c_str());
        return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"macports install failed":"" };
    }
    Result remove(const Package& pkg) override {
        int r = std::system(("macports remove " + pkg.name).c_str());
        return { r==0, "", r!=0?"macports remove failed":"" };
    }
    SearchResult search(const std::string& q) override {
        std::system(("macports search " + q).c_str()); return { {}, true, "" };
    }
    Result update() override {
        int r = std::system("macports update"); return { r==0, "", "" };
    }
    Result upgrade() override {
        int r = std::system("macports upgrade"); return { r==0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }
};

} // namespace sspm::backends
