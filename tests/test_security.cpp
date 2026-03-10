#include "sspm/security.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

TEST(SecurityTest, Sha256KnownValue) {
    std::string path = "/tmp/test_hash.txt";
    std::ofstream f(path);
    f << "hello";
    f.close();
    // SHA-256("hello") = 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    std::string hash = sspm::security::sha256(path);
    EXPECT_EQ(hash, "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
    fs::remove(path);
}

TEST(SecurityTest, VerifyHashMatch) {
    std::string path = "/tmp/test_hash2.txt";
    std::ofstream f(path);
    f << "sspm";
    f.close();
    std::string hash = sspm::security::sha256(path);
    EXPECT_TRUE(sspm::security::verify_hash(path, hash));
    EXPECT_FALSE(sspm::security::verify_hash(path, "deadbeef"));
    fs::remove(path);
}
