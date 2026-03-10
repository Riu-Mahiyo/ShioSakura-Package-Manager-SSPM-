#include "sspm/cache.h"
#include <gtest/gtest.h>
#include <filesystem>

namespace fs = std::filesystem;

class CacheTest : public ::testing::Test {
protected:
    std::string root = "/tmp/test_sspm_cache";
    void SetUp() override { fs::remove_all(root); }
    void TearDown() override { fs::remove_all(root); }
};

TEST_F(CacheTest, StoreAndRetrieve) {
    sspm::CacheManager cache(root);
    std::string url = "https://example.com/package.spk";
    std::string data = "fake package data";
    EXPECT_TRUE(cache.store(url, data));
    EXPECT_TRUE(cache.has(url));
}

TEST_F(CacheTest, SizeAfterStore) {
    sspm::CacheManager cache(root);
    cache.store("https://example.com/a.spk", std::string(1024, 'x'));
    EXPECT_GT(cache.total_size(), 0);
}

TEST_F(CacheTest, Clean) {
    sspm::CacheManager cache(root);
    cache.store("https://example.com/a.spk", "data");
    EXPECT_TRUE(cache.clean());
    EXPECT_EQ(cache.total_size(), 0);
}
