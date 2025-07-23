#ifndef MYTEST_INTERNAL_HPP_
#define MYTEST_INTERNAL_HPP_

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "mytest.hpp"

namespace mytest {

struct TestManager {
    void add_test_case(const TestCase &test_case);

    int run_tests();
    void report_failure();
    std::vector<std::string> &get_running_subcases();

    void increase_depth();
    void increase_subcases_amount();
    bool is_up_to_execute();
    void push_to_stack(const std::string &name);
    void clear_level();
    void increase_traversed_subcases();
    void decrease_depth();
    void block_level();
    bool has_fully_traversed();

private:
    std::vector<TestCase> test_cases;

    void start_new_test_case();
    void start_new_iteration();
    bool is_continuable();

    unsigned tests_total = 0;
    unsigned tests_passed = 0;

    int subcases_depth = 0;
    std::unordered_map<int, bool> is_blocked;
    std::unordered_map<int, int> subcases_traversed;
    std::unordered_map<int, int> subcases_amount;
    std::vector<std::string> running_subcases_stack;

    bool current_test_failed = false;
};

bool operator<(const TestCase &lhs, const TestCase &rhs);

TestManager &get_test_manager();

}  // namespace mytest

#endif  // MYTEST_INTERNAL_HPP