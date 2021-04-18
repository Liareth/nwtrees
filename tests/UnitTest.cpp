#include "UnitTest.hpp"

#include <nwtrees/util/Assert.hpp>

#if defined(WIN32)
    #include <Windows.h>
#endif

#include <atomic>
#include <chrono>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

std::vector<test::UnitTest>& get_tests()
{
    static std::vector<test::UnitTest> s_tests;
    return s_tests;
}

void test::register_unit_test(const char* class_name, const char* test_name, UnitTestFunc function)
{
    get_tests().push_back({ class_name, test_name, function });
}

void test::debug_break()
{
#if defined(WIN32)
    if (IsDebuggerPresent())
    {
        __debugbreak();
    }
#endif
}

static std::atomic<int64_t> s_bytes_allocated = 0;
static std::atomic<int64_t> s_total_bytes_allocated = 0;

void* custom_alloc(size_t size)
{
    void* data = ::malloc(size + 8);
    memcpy(data, &size, 8);
    s_bytes_allocated += size;
    s_total_bytes_allocated += size;
    return (unsigned char*)data + 8;
}

void custom_free(void* data)
{
    if (data)
    {
        size_t size;
        void* data_start = (unsigned char*)data - 8;
        memcpy(&size, data_start, 8);
        ::free(data_start);
        s_bytes_allocated -= size;
    }
}

void* operator new(size_t size)
{
    return custom_alloc(size);
}

void operator delete(void* data) noexcept
{
    custom_free(data);
}

void operator delete(void* data, size_t) noexcept
{
    custom_free(data);
}

int main(int argc, char** argv)
{
    using namespace test;

    char* whitelist = argc == 2 ? argv[1] : nullptr;

    int64_t overall_bytes_before = s_bytes_allocated;
    bool any_failures = false;

    for (const UnitTest& test : get_tests())
    {
        char name_buf[4096];
        sprintf(name_buf, "%s_%s", test.class_name, test.test_name);

        if (whitelist && !strstr(name_buf, whitelist))
        {
            printf("Skipping test %s\n", name_buf);
            continue;
        }

        printf("Running test %s ...", name_buf);
        fflush(stdout);

        nwtrees::assert::g_fail_count = 0;

        const int64_t total_bytes_before = s_total_bytes_allocated;
        const int64_t bytes_before = s_bytes_allocated;
        const auto time_before = std::chrono::high_resolution_clock::now();

        UnitTestResult result = test.function();

        const auto time_after = std::chrono::high_resolution_clock::now();
        const int64_t bytes_after = s_bytes_allocated;

        if (result.failed_condition)
        {
            printf(" FAILED!\n    %s:%d\n    %s\n", result.failed_file, result.failed_line, result.failed_condition);
            any_failures = true;
        }
        else if (bytes_before != bytes_after)
        {
            printf(" FAILED!\n    %" PRId64 " bytes before test, %" PRId64 " bytes after test. Memory leak?\n", bytes_before, bytes_after);
            any_failures = true;
        }
        else if (nwtrees::assert::g_fail_count)
        {
            printf(" FAILED!\n    Failed %d asserts.\n", nwtrees::assert::g_fail_count);
            any_failures = true;
        }
        else
        {
            printf(" SUCCESS! (%.2f ms, alloc: %" PRId64 ")\n",
                std::chrono::duration_cast<std::chrono::nanoseconds>(time_after - time_before).count() / 1000.0f / 1000.0f,
                s_total_bytes_allocated - total_bytes_before);
        }

        fflush(stdout);

    }

    int64_t bytes_remaining = s_bytes_allocated;

    if (bytes_remaining != overall_bytes_before)
    {
        printf("FAILED!\n    %" PRId64 " bytes were still allocated at teardown; expected %" PRId64 ".\n", bytes_remaining, overall_bytes_before);
        fflush(stdout);
        any_failures = true;
    }

    if (any_failures)
    {
        debug_break();
        return 1;
    }
}
