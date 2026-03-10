#pragma once
#include "sspm/module.h"
#include <string>
#include <vector>
#include <optional>

namespace sspm {

struct Package {
    std::string name;
    std::string version;
    std::string description;
    std::string backend;
    std::string repo;
    std::string platform;
    std::string hash;
    std::string install_time;
    bool desktop_registered = false;
    bool mime_registered    = false;
};

struct SearchResult {
    std::vector<Package> packages;
    bool success;
    std::string error;
};

struct Result {
    bool success;
    std::string message;
    std::string error;
};

using PackageList = std::vector<Package>;

// Abstract base class every backend must implement, now inheriting from IModule
class Backend : public IModule {
public:
    virtual ~Backend() = default;

    // From IModule
    virtual std::string id() const override { return "backend." + name(); }
    virtual Version version() const override { return { 1, 0, 0, "Sakura" }; }
    virtual bool initialize(const std::map<std::string, std::string>& context) override { return true; }
    virtual void shutdown() override {}

    virtual std::string name() const = 0;
    virtual bool is_available() const = 0;

    virtual Result install(const Package& pkg) = 0;
    virtual Result remove(const Package& pkg) = 0;
    virtual SearchResult search(const std::string& query) = 0;
    virtual Result update() = 0;
    virtual Result upgrade() = 0;
    virtual PackageList list_installed() = 0;
    virtual std::optional<Package> info(const std::string& name) = 0;
};


} // namespace sspm
