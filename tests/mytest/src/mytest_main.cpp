#include <algorithm>
#include <iostream>
#include "mytest.hpp"
#include "mytest_internal.hpp"

namespace mytest {
int TestManager::run_tests() {
    std::sort(test_cases.begin(), test_cases.end());
    tests_total = test_cases.size();

    for (const auto &test_case : test_cases) {
        start_new_test_case();
        // Colorized start
        std::cerr << "\033[1;34mRunning " << test_case.name << "...\033[0m" << std::endl;
        test_case.func();

        while (is_continuable()) {
            start_new_iteration();
            std::cerr << "\033[1;33m...another subcase... \033[0m" << std::endl;
            test_case.func();
        }

        if (!current_test_failed) {
            tests_passed++;
            std::cerr << "\033[1;32m[PASS]\033[0m " << test_case.name << std::endl;
        } else {
            std::cerr << "\033[1;31m[FAIL]\033[0m " << test_case.name << std::endl;
        }
    }

    // Summary
    if (tests_passed == tests_total) {
        std::cerr << "\n\033[1;32m===== All tests passed: " \
                  << tests_passed << "/" << tests_total << " =====\033[0m" << std::endl;
    } else {
        std::cerr << "\n\033[1;31m===== Tests passed: " \
                  << tests_passed << "/" << tests_total << " =====\033[0m" << std::endl;
    }
    return tests_passed == tests_total ? 0 : 1;
}
}  // namespace mytest

int main() {
    return ::mytest::get_test_manager().run_tests();
}