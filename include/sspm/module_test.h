#pragma once
#include "sspm/module.h"
#include <cassert>
#include <iostream>

namespace sspm::test {

/**
 * @brief Base class for module unit tests.
 */
class ModuleTester {
public:
    virtual ~ModuleTester() = default;

    void run_all() {
        std::cout << "🧪 Testing module: " << module_name() << "...\n";
        setup();
        run_tests();
        teardown();
        std::cout << "✅ All tests passed for " << module_name() << "\n";
    }

protected:
    virtual std::string module_name() const = 0;
    virtual void setup() {}
    virtual void teardown() {}
    virtual void run_tests() = 0;

    void assert_true(bool condition, const std::string& msg) {
        if (!condition) {
            std::cerr << "❌ Assertion failed: " << msg << "\n";
            exit(1);
        }
    }
};

} // namespace sspm::test
