find_package(GTest QUIET)

if(GTest_FOUND)
    add_executable(sspm_tests
        test_database.cpp
        test_cache.cpp
        test_index.cpp
        test_resolver.cpp
        test_security.cpp
        test_transaction.cpp
        test_apk_backend.cpp
    )
    target_include_directories(sspm_tests PRIVATE ../include)
    target_link_libraries(sspm_tests GTest::gtest_main SQLite3::SQLite3 CURL::libcurl OpenSSL::SSL OpenSSL::Crypto)
    include(GoogleTest)
    gtest_discover_tests(sspm_tests)
else()
    message(STATUS "GTest not found — skipping unit tests")
endif()
