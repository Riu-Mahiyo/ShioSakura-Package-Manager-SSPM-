#include "sspm/resolver.h"
#include <gtest/gtest.h>

// Minimal mock backend for testing
class MockBackend : public sspm::Backend {
public:
    explicit MockBackend(const std::string& name, bool avail,
                         const std::vector<std::string>& pkgs)
        : name_(name), avail_(avail), pkgs_(pkgs) {}

    std::string name() const override { return name_; }
    bool is_available() const override { return avail_; }

    sspm::Result install(const sspm::Package&) override { return { true, "", "" }; }
    sspm::Result remove(const sspm::Package&) override { return { true, "", "" }; }
    sspm::Result update() override { return { true, "", "" }; }
    sspm::Result upgrade() override { return { true, "", "" }; }
    sspm::PackageList list_installed() override { return {}; }
    std::optional<sspm::Package> info(const std::string&) override { return std::nullopt; }

    sspm::SearchResult search(const std::string& q) override {
        sspm::SearchResult r;
        r.success = true;
        for (auto& p : pkgs_) {
            if (p == q) r.packages.push_back({ p, "1.0", "", name_, "" });
        }
        return r;
    }

private:
    std::string name_;
    bool avail_;
    std::vector<std::string> pkgs_;
};

TEST(ResolverTest, SelectsAvailableBackend) {
    auto b1 = std::make_shared<MockBackend>("apt",    true,  std::vector<std::string>{"nginx"});
    auto b2 = std::make_shared<MockBackend>("pacman", false, std::vector<std::string>{"nginx"});

    sspm::Resolver resolver({ b1, b2 });
    auto selected = resolver.resolve("nginx");
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->name(), "apt");
}

TEST(ResolverTest, RespectsUserPriority) {
    auto b1 = std::make_shared<MockBackend>("apt",     true, std::vector<std::string>{"vim"});
    auto b2 = std::make_shared<MockBackend>("flatpak", true, std::vector<std::string>{"vim"});

    sspm::Resolver resolver({ b1, b2 });
    resolver.set_priority({ "flatpak", "apt" });
    auto selected = resolver.resolve("vim");
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->name(), "flatpak");
}

TEST(ResolverTest, ReturnsNullWhenNotFound) {
    auto b1 = std::make_shared<MockBackend>("apt", true, std::vector<std::string>{"curl"});
    sspm::Resolver resolver({ b1 });
    auto selected = resolver.resolve("unknownpkg999");
    EXPECT_EQ(selected, nullptr);
}
