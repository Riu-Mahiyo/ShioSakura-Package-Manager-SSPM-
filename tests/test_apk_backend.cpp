#include "sspm/backend_registry.h"
#include "sspm/backends/apk_backend.h"
#include <gtest/gtest.h>

using namespace sspm;
using namespace sspm::backends;

// helper to create a fake command runner that returns a predetermined code
static ApkBackend::CmdFn make_runner(const std::map<std::string,int>& m) {
    return [m](const std::string& cmd) {
        for (auto& kv : m) {
            if (cmd.find(kv.first) != std::string::npos) return kv.second;
        }
        // default to success so tests that don't care still progress
        return 0;
    };
}

TEST(ApkBackendTest, DisabledWhenNotAlpine) {
    BackendRegistry::set_test_force_alpine(false);
    ApkBackend::set_cmd_fn(make_runner({{"which apk",0}}));
    ApkBackend backend;
    EXPECT_FALSE(backend.is_available());
    auto r = backend.install({"foo", ""});
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error, "apk is not supported on this system");
}

TEST(ApkBackendTest, AvailableOnAlpine) {
    BackendRegistry::set_test_force_alpine(true);
    ApkBackend::set_cmd_fn(make_runner({{"which apk",0}}));
    ApkBackend backend;
    EXPECT_TRUE(backend.is_available());

    // override exec_capture so we can fake output directly
    ApkBackend::set_exec_fn([&](const std::string& cmd) {
        if (cmd.find("apk info -v") != std::string::npos) {
            return std::make_pair(0, std::string("foo-1.0\nbar-2.3.4\n"));
        }
        return std::make_pair(0, std::string());
    });
    auto list = backend.list_installed();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list[0].name, "foo");
    EXPECT_EQ(list[0].version, "1.0");
    EXPECT_EQ(list[1].name, "bar");
    EXPECT_EQ(list[1].version, "2.3.4");

    // info() should parse a single line
    ApkBackend::set_exec_fn([&](const std::string& cmd) {
        if (cmd.find("apk info -v baz") != std::string::npos) {
            return std::make_pair(0, std::string("baz-9.9\n"));
        }
        return std::make_pair(1, std::string());
    });
    auto info = backend.info("baz");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "baz");
    EXPECT_EQ(info->version, "9.9");

    // search should return parsed packages
    ApkBackend::set_exec_fn([&](const std::string& cmd) {
        if (cmd.find("apk search -v query") != std::string::npos) {
            return std::make_pair(0, std::string("foo-1\nbar-2\n"));
        }
        return std::make_pair(0, std::string());
    });
    auto res = backend.search("query");
    EXPECT_TRUE(res.success);
    ASSERT_EQ(res.packages.size(), 2);
    EXPECT_EQ(res.packages[1].name, "bar");
}

TEST(BackendRegistryTest, ApkOnlyOnAlpine) {
    BackendRegistry reg = BackendRegistry::create();

    // not Alpine -> should not appear
    BackendRegistry::set_test_force_alpine(false);
    auto avail = reg.available_backends();
    bool found = std::any_of(avail.begin(), avail.end(), [](auto& b){return b->name()=="apk";});
    EXPECT_FALSE(found);

    // force Alpine -> registry should include it when probed
    BackendRegistry::set_test_force_alpine(true);
    ApkBackend::set_cmd_fn(make_runner({{"which apk",0}}));
    reg.refresh(); // re-probe after override
    avail = reg.available_backends();
    found = std::any_of(avail.begin(), avail.end(), [](auto& b){return b->name()=="apk";});
    EXPECT_TRUE(found);
}

// helper tests for detection utilities
TEST(BackendRegistryTest, DetectionHelpers) {
    // reset flags
    BackendRegistry::set_test_force_alpine(false);
    BackendRegistry::set_test_force_musl(false);
    EXPECT_FALSE(BackendRegistry::is_alpine());
    EXPECT_FALSE(BackendRegistry::is_musl());

    BackendRegistry::set_test_force_alpine(true);
    EXPECT_TRUE(BackendRegistry::is_alpine());
    EXPECT_TRUE(BackendRegistry::is_musl());   // alpine implies musl

    BackendRegistry::set_test_force_alpine(false);
    BackendRegistry::set_test_force_musl(true);
    EXPECT_FALSE(BackendRegistry::is_alpine());
    EXPECT_TRUE(BackendRegistry::is_musl());
}
