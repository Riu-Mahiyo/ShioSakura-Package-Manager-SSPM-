#include "sspm/database.h"
#include <gtest/gtest.h>
#include <filesystem>

namespace fs = std::filesystem;

class SkyDBTest : public ::testing::Test {
protected:
    std::string db_path = "/tmp/test_sky.db";
    void SetUp() override {
        fs::remove(db_path);
    }
    void TearDown() override {
        fs::remove(db_path);
    }
};

TEST_F(SkyDBTest, OpenAndInitSchema) {
    sspm::SkyDB db(db_path);
    EXPECT_TRUE(db.open());
}

TEST_F(SkyDBTest, InsertAndGetPackage) {
    sspm::SkyDB db(db_path);
    ASSERT_TRUE(db.open());

    sspm::Package pkg{ "nginx", "1.25.0", "HTTP server", "apt", "official" };
    EXPECT_TRUE(db.insert_package(pkg));

    auto found = db.get_package("nginx");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "nginx");
    EXPECT_EQ(found->version, "1.25.0");
    EXPECT_EQ(found->backend, "apt");
}

TEST_F(SkyDBTest, RemovePackage) {
    sspm::SkyDB db(db_path);
    ASSERT_TRUE(db.open());

    sspm::Package pkg{ "curl", "8.0", "HTTP client", "pacman", "official" };
    db.insert_package(pkg);
    EXPECT_TRUE(db.remove_package("curl"));
    EXPECT_FALSE(db.get_package("curl").has_value());
}

TEST_F(SkyDBTest, ListInstalled) {
    sspm::SkyDB db(db_path);
    ASSERT_TRUE(db.open());

    db.insert_package({ "git",  "2.43", "VCS",         "apt",    "official" });
    db.insert_package({ "vim",  "9.0",  "Text editor", "pacman", "official" });

    auto list = db.get_installed();
    EXPECT_EQ(list.size(), 2);
}

TEST_F(SkyDBTest, AddAndListRepos) {
    sspm::SkyDB db(db_path);
    ASSERT_TRUE(db.open());

    EXPECT_TRUE(db.add_repo("official", "https://repo.sspm.dev"));
    auto repos = db.get_repos();
    EXPECT_EQ(repos.size(), 1);
    EXPECT_EQ(repos[0].first, "official");
}

TEST_F(SkyDBTest, Integrity) {
    sspm::SkyDB db(db_path);
    ASSERT_TRUE(db.open());
    EXPECT_TRUE(db.check_integrity());
}
