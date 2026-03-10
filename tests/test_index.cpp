#include "sspm/index.h"
#include <gtest/gtest.h>
#include <fstream>

class IndexTest : public ::testing::Test {
protected:
    sspm::PackageIndex index;

    void SetUp() override {
        // Inject test entries directly (bypass file loading)
        // In real tests, write a JSON fixture to /tmp and call index.load()
    }
};

TEST_F(IndexTest, FuzzySearchExactMatch) {
    // Load a small JSON fixture
    std::string json_path = "/tmp/test_index.json";
    std::ofstream f(json_path);
    f << R"([
        {"name":"nginx","version":"1.25","description":"HTTP server"},
        {"name":"nmap","version":"7.94","description":"Network scanner"},
        {"name":"neovim","version":"0.9","description":"Text editor"}
    ])";
    f.close();

    index.load(json_path, "apt");
    auto results = index.search_fuzzy("nginx", 5);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].name, "nginx");
}

TEST_F(IndexTest, FuzzySearchPartial) {
    std::string json_path = "/tmp/test_index2.json";
    std::ofstream f(json_path);
    f << R"([{"name":"firefox","version":"121","description":"Web browser"}])";
    f.close();

    index.load(json_path, "apt");
    auto results = index.search_fuzzy("fire", 5);
    EXPECT_FALSE(results.empty());
}

TEST_F(IndexTest, RegexSearch) {
    std::string json_path = "/tmp/test_index3.json";
    std::ofstream f(json_path);
    f << R"([
        {"name":"python3","version":"3.12","description":"Python interpreter"},
        {"name":"python2","version":"2.7","description":"Legacy Python"}
    ])";
    f.close();

    index.load(json_path, "apt");
    auto results = index.search_regex("python[0-9]");
    EXPECT_EQ(results.size(), 2);
}
