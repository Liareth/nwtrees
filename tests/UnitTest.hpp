#pragma once

#include <cstddef>

namespace test
{

struct UnitTestResult
{
    const char* failed_condition = nullptr;
    const char* failed_file;
    int failed_line;
};

using UnitTestFunc = UnitTestResult(*)();

struct UnitTest
{
    const char* class_name;
    const char* test_name;
    UnitTestFunc function;
};

void register_unit_test(const char* class_name, const char* test_name, UnitTestFunc function);

struct ScopedUnitTest{
    ScopedUnitTest(const char* class_name, const char* test_name, UnitTestFunc function)
    {
        register_unit_test(class_name, test_name, function);
    }
};

template <typename T>
struct TestBase
{
    using _TestType = T;
    UnitTestResult test_result;
};

template <typename T>
struct TestName
{
    constexpr TestName(const char* name) { str = name; }
    static inline const char* str;
};

#define _TEST_CLASS(name)                             \
    class name;                                       \
    static ::test::TestName<name> _reg_##name(#name); \
    class name : ::test::TestBase<name>

#define _TEST_METHOD(name)                               \
    static ::test::UnitTestResult _invoke_##name()       \
    {                                                    \
        _TestType test;                                  \
        test._##name();                                  \
        return test.test_result;                         \
    }                                                    \
    static inline ::test::ScopedUnitTest _invoker_##name \
    {                                                    \
        ::test::TestName<_TestType>::str,                \
        #name,                                           \
        &_invoke_##name                                  \
    };                                                   \
    void _##name()

#define TEST_CLASS(name) _TEST_CLASS(Test_##name)
#define TEST_METHOD(name) _TEST_METHOD(name)
#define BENCHMARK_CLASS(name) _TEST_CLASS(Benchmark_##name)
#define BENCHMARK_METHOD(name) _TEST_METHOD(name)

void debug_break();

#define TEST_EXPECT(cond)                             \
    do                                                \
    {                                                 \
        if (!(cond) && !test_result.failed_condition) \
        {                                             \
            ::test::debug_break();                    \
            test_result.failed_condition = (#cond);   \
            test_result.failed_file = __FILE__;       \
            test_result.failed_line = __LINE__;       \
        }                                             \
    } while (0)
}
