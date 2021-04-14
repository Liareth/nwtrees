#pragma once

#include <stdio.h>

namespace nwtrees::assert
{
    template <typename ... Args>
    void fail(const char* condition, const char* file, int line, const char* format, Args ... args)
    {
        printf("ASSERTION FAILURE\n  Summary: ");
        printf(format, args ...);

        if (condition)
        {
            printf("\n  Condition (%s) failed at (%s:%d)\n  ", condition, file, line);
        }
        else
        {
            printf("\n  Failed at (%s:%d)\n  ", file, line);
        }

#if defined(_WIN32)
        __debugbreak();
#endif
    }

#if defined(_DEBUG)
    #define NWTREES_ASSERT(condition) \
        NWTREES_ASSERT_MSG(condition, "%s", "(no message)")

    #define NWTREES_ASSERT_MSG(condition, format, ...) \
        do \
        { \
            if (!(condition)) ::nwtrees::assert::fail((#condition), __FILE__, __LINE__, (format), __VA_ARGS__); \
        } \
        while (0)

    #define NWTREES_ASSERT_FAIL() \
        ::nwtrees::assert::fail(nullptr, __FILE__, __LINE__, nullptr)

    #define NWTREES_ASSERT_FAIL_MSG(format, ...) \
        ::nwtrees::assert::fail(nullptr, __FILE__, __LINE__, (format), __VA_ARGS__)
#else
    #define NWTREES_ASSERT(condition) (void)0
    #define NWTREES_ASSERT_MSG(condition, format, ...) (void)0
    #define NWTREES_ASSERT_FAIL() (void)0
    #define NWTREES_ASSERT_FAIL_MSG(format, ...) (void)0
#endif

}
