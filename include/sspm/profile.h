#pragma once
#include <string>
#include <vector>

namespace sspm {

struct Profile {
    std::string name;
    std::vector<std::string> packages;
    std::string created_at;
};

// Manages named package profiles (dev, desktop, server, gaming…)
// Each profile is a named list of packages that can be applied atomically.
class ProfileManager {
public:
    explicit ProfileManager(const std::string& db_path);

    // sspm profile create <n>
    bool create(const std::string& name, const std::vector<std::string>& packages = {});

    // sspm profile apply <n> — install all packages in profile
    bool apply(const std::string& name);

    // sspm profile list
    std::vector<Profile> list() const;

    // sspm profile delete <n>
    bool remove(const std::string& name);

    // Add a package to an existing profile
    bool add_package(const std::string& profile, const std::string& pkg);

private:
    std::string db_path_;
};

} // namespace sspm
