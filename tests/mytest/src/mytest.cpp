#include "mytest.hpp"
#include <iostream>
#include <vector>
#include "mytest_internal.hpp"

namespace mytest {

void TestManager::add_test_case(const TestCase &test_case) {
    test_cases.push_back(test_case);
}

void TestManager::report_failure() {
    current_test_failed = true;
}

std::vector<std::string> &TestManager::get_running_subcases() {
    return running_subcases_stack;
}

void TestManager::increase_depth() {
    subcases_depth++;
}

void TestManager::increase_subcases_amount() {
    subcases_amount[subcases_depth]++;
}

bool TestManager::is_up_to_execute() {
    return !is_blocked[subcases_depth] &&
           subcases_amount[subcases_depth] > subcases_traversed[subcases_depth];
}

void TestManager::push_to_stack(const std::string &name) {
    running_subcases_stack.push_back(name);
}

void TestManager::clear_level() {
    subcases_traversed[subcases_depth] = 0;
    subcases_amount[subcases_depth] = 0;
}

void TestManager::decrease_depth() {
    subcases_depth--;
}

void TestManager::block_level() {
    is_blocked[subcases_depth] = true;
}

void TestManager::increase_traversed_subcases() {
    subcases_traversed[subcases_depth - 1]++;
}

bool TestManager::has_fully_traversed() {
    return subcases_amount[subcases_depth] ==
           subcases_traversed[subcases_depth];
}

void TestManager::start_new_test_case() {
    current_test_failed = false;
    subcases_traversed.clear();
}

void TestManager::start_new_iteration() {
    is_blocked.clear();
    subcases_amount.clear();
    running_subcases_stack.clear();
}

bool TestManager::is_continuable() {
    return subcases_traversed[0] != subcases_amount[0] && !current_test_failed;
}

// cppcheck-suppress[unusedFunction]
void check(
    bool expr,
    const std::string &expr_str,
    const std::string &file,
    int line,
    std::optional<std::string> msg
) {
    if (expr) {
        return;
    }
    ::mytest::get_test_manager().report_failure();
    // Colorized error output
    std::cerr << "\033[1;31mCHECK(" << expr_str << ") failed at "
              << file << ":" << line << "\033[0m" << std::endl;
    if (msg.has_value()) {
        std::cerr << "    \033[33mmessage:\033[0m " << msg.value() << std::endl;
    }
    for (const auto &subcase : get_test_manager().get_running_subcases()) {
        std::cerr << "    \033[36min subcase:\033[0m " << subcase << std::endl;
    }
}

// cppcheck-suppress[unusedFunction]
void register_test(func_ptr func, const std::string &name) {
    get_test_manager().add_test_case(TestCase{func, name});
}

bool operator<(const TestCase &lhs, const TestCase &rhs) {
    return lhs.name < rhs.name;
}

Subcase::Subcase(const std::string &name) {
    auto &test_manager = get_test_manager();
    test_manager.increase_subcases_amount();
    if (!test_manager.is_up_to_execute()) {
        return;
    }
    test_manager.push_to_stack(name);
    test_manager.increase_depth();
    executing_ = true;
}

Subcase::~Subcase() {
    auto &test_manager = get_test_manager();
    if (executing_) {
        if (test_manager.has_fully_traversed()) {
            test_manager.clear_level();
            test_manager.increase_traversed_subcases();
        }
        test_manager.decrease_depth();
        test_manager.block_level();
    }
}

Subcase::operator bool() const {
    return executing_;
}

TestManager &get_test_manager() {
    static TestManager test_manager;
    return test_manager;
}

}  // namespace mytest
