#include "sspm/backend.h"
#include <cstdlib>

namespace sspm::backends {

class wingetBackend : public Backend {
public:
    std::string name() const override { return "winget"; }
    bool is_available() const override {
        return std::system("which winget > /dev/null 2>&1") == 0;
    }
    Result install(const Package& pkg) override {
        int r = std::system(("winget install " + pkg.name).c_str());
        return { r==0, r==0?"Installed "+pkg.name:"", r!=0?"winget install failed":"" };
    }
    Result remove(const Package& pkg) override {
        int r = std::system(("winget remove " + pkg.name).c_str());
        return { r==0, "", r!=0?"winget remove failed":"" };
    }
    SearchResult search(const std::string& q) override {
        std::system(("winget search " + q).c_str()); return { {}, true, "" };
    }
    Result update() override {
        int r = std::system("winget update"); return { r==0, "", "" };
    }
    Result upgrade() override {
        int r = std::system("winget upgrade"); return { r==0, "", "" };
    }
    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string& n) override { return std::nullopt; }
};

} // namespace sspm::backends
