#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <ostream>

namespace sspm {

/**
 * @brief Semantic version representation for modules and plugins.
 */
struct Version {
    int major;
    int minor;
    int patch;
    std::string prerelease;

    std::string to_string() const {
        std::string s = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        if (!prerelease.empty()) s += "-" + prerelease;
        return s;
    }

    bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Version& v) {
    return os << v.to_string();
}

/**
 * @brief Base interface for all system modules and plugins.
 * 
 * Each module in the system (Backend, UI, Network, etc.) must implement this interface
 * to support lifecycle management and dynamic discovery.
 */
class IModule {
public:
    virtual ~IModule() = default;

    /** @return Unique identifier for the module (e.g., "core.db", "backend.apt"). */
    virtual std::string id() const = 0;

    /** @return Human-readable name of the module. */
    virtual std::string name() const = 0;

    /** @return Current version of the module. */
    virtual Version version() const = 0;

    /**
     * @brief Initialize the module.
     * @param context Key-value pairs for initialization (config, paths, etc.).
     * @return true if initialization succeeded.
     */
    virtual bool initialize(const std::map<std::string, std::string>& context) = 0;

    /**
     * @brief Clean up and shut down the module.
     */
    virtual void shutdown() = 0;

    /** @return Current health/status of the module (e.g., "OK", "ERROR: Connection failed"). */
    virtual std::string status() const { return "OK"; }

    /** @return Detailed metrics/stats for monitoring (JSON format). */
    virtual std::string metrics() const { return "{}"; }
};

/**
 * @brief Module metadata used for dependency management and discovery.
 */
struct ModuleInfo {
    std::string id;
    Version version;
    std::vector<std::string> dependencies; // List of module IDs this module depends on
    std::string description;
    std::string author;
};

/**
 * @brief Configuration context for modules.
 */
class ConfigManager {
public:
    virtual ~ConfigManager() = default;
    virtual std::string get(const std::string& key, const std::string& default_val = "") const = 0;
    virtual void set(const std::string& key, const std::string& val) = 0;
    virtual std::map<std::string, std::string> get_all_for_module(const std::string& module_id) const = 0;
};

} // namespace sspm
