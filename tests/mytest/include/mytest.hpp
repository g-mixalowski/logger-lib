#ifndef MYTEST_HPP_
#define MYTEST_HPP_

#include <optional>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define MYTEST_INTERNAL_CAT_IMPL(s1, s2) s1##s2
#define MYTEST_INTERNAL_CAT(s1, s2) MYTEST_INTERNAL_CAT_IMPL(s1, s2)
#define MYTEST_INTERNAL_ANONYMOUS(x) MYTEST_INTERNAL_CAT(x, __LINE__)
#define MYTEST_INTERNAL_RUN_EXPR_BEFORE_MAIN(expr)        \
    namespace {                                           \
    const int MYTEST_INTERNAL_ANONYMOUS(MYTEST_ANON_VAR_) \
        [[maybe_unused]] = ((expr), 0);                   \
    }
#define MYTEST_INTERNAL_REGISTER_FUNCTION(func, name) \
    MYTEST_INTERNAL_RUN_EXPR_BEFORE_MAIN(             \
        ::mytest::register_test(&(func), #name)       \
    )
#define MYTEST_INTERNAL_CREATE_AND_REGISTER_FUNCTION(func, name) \
    static void func();                                          \
    MYTEST_INTERNAL_REGISTER_FUNCTION(func, name);               \
    static void func()

#define CHECK(expr) \
    ::mytest::check(bool(expr), #expr, __FILE__, __LINE__, ::std::nullopt)
#define CHECK_MESSAGE(expr, msg) \
    ::mytest::check(bool(expr), #expr, __FILE__, __LINE__, msg)

#define TEST_CASE_REGISTER(func, name) \
    MYTEST_INTERNAL_REGISTER_FUNCTION(func, name)
#define TEST_CASE(name)                                             \
    MYTEST_INTERNAL_CREATE_AND_REGISTER_FUNCTION(                   \
        MYTEST_INTERNAL_ANONYMOUS(MYTEST_INTERNAL_ANON_FUNC_), name \
    )

#define SUBCASE(name)                                         \
    if (const ::mytest::Subcase &                             \
            MYTEST_INTERNAL_ANONYMOUS(MYTEST_ANON_SUBCASE_) = \
            ::mytest::Subcase(name))

// NOLINTEND(cppcoreguidelines-macro-usage)

namespace mytest {

using func_ptr = void (*)();

struct Subcase {
    explicit Subcase(const std::string &name);
    ~Subcase();

    Subcase(const Subcase &) = delete;
    Subcase(Subcase &&) = delete;
    Subcase operator=(const Subcase &) = delete;
    Subcase operator=(Subcase &&) = delete;

    operator bool() const;

private:
    bool executing_ = false;
};

struct TestCase {
    func_ptr func;
    std::string name;
};

void check(
    bool expr,
    const std::string &expr_str,
    const std::string &file,
    int line,
    std::optional<std::string> msg
);

void register_test(func_ptr func, const std::string &name);

}  // namespace mytest

#endif  // MYTEST_HPP_