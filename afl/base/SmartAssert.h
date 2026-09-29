/**
 * @file   SmartAssert.h
 * @brief  增强版的 Assert 实现
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <string>
#include <sstream>
#include <stdlib.h>
namespace afl
{
namespace base
{
#define ENABLE_SMART_ASSERT_MODE //enable AFL_ASSERT macro, use in Debug/Release env

#define ABORT_IF_ASSERT_FAILED // if assert failed, abort(), except AFL_ASSERT_LOG

class SmartAssert
{
public:
    SmartAssert(const char* expr, const char* function, int line, const char* file,
                bool abortOnExit = false)
        : SMART_ASSERT_A(*this), SMART_ASSERT_B(*this), m_abortIfExit(abortOnExit)
    {
        std::ostringstream oss;
        if (expr && *expr)
            oss << "Expression Failed: " << expr << "\n";
        if (function && *function)
            oss << "Failed in [func: " << function << "], [line: " << line << "], [file: " << file
                << "]\n";
        m_errMsg += oss.str();
    }

    ~SmartAssert()
    {
        std::cerr << m_errMsg << "\n";

        if (m_abortIfExit)
        {
#if defined(ABORT_IF_ASSERT_FAILED)
            abort();
#endif
        }
    }

    template <typename T>
    SmartAssert& printValiable(const char* expr, const T& value)
    {
        std::ostringstream oss;
        oss << "ContextValiable: [" << expr << " = " << value << "]\n";
        m_errMsg += oss.str();
        return *this;
    }

public:
    SmartAssert& SMART_ASSERT_A;
    SmartAssert& SMART_ASSERT_B;

private:
    bool m_abortIfExit;
    std::string m_errMsg;
};

static SmartAssert MakeAssert(const char* expr, const char* function, int line, const char* file,
                              bool abortOnExit)
{
    return afl::base::SmartAssert(expr, function, line, file, abortOnExit);
}

static SmartAssert __dont_use_this__ =
    MakeAssert(NULL, NULL, 0, 0,
               false); //gcc: MakeAssert 定义未使用[-Wunused-function]

// 运行时断言
#ifndef ENABLE_SMART_ASSERT_MODE
#define AFL_ASSERT(expr) ((void)0)
#define AFL_ASSERTEX(expr, func, lineno, file) ((void)0)
#define AFL_ASSERT_LOG(expr) ((void)0)
#else
#define SMART_ASSERT_A(x) SMART_ASSERT_OP(x, B)
#define SMART_ASSERT_B(x) SMART_ASSERT_OP(x, A)
#define SMART_ASSERT_OP(x, next) SMART_ASSERT_A.printValiable(#x, (x)).SMART_ASSERT_##next

#define AFL_ASSERT(expr)                                                                           \
    if ((expr))                                                                                    \
        ;                                                                                          \
    else                                                                                           \
        afl::base::MakeAssert(#expr, __FUNCTION__, __LINE__, __FILE__, true).SMART_ASSERT_A
#define AFL_ASSERTEX(expr, func, lineno, file)                                                     \
    if ((expr))                                                                                    \
        ;                                                                                          \
    else                                                                                           \
        afl::base::MakeAssert(#expr, func, lineno, file, true).SMART_ASSERT_A
#define AFL_ASSERT_LOG(expr)                                                                       \
    if ((expr))                                                                                    \
        ;                                                                                          \
    else                                                                                           \
        afl::base::MakeAssert(#expr, __FUNCTION__, __LINE__, __FILE__, false).SMART_ASSERT_A
#endif

// 编译期断言
#ifdef AFL_CXX11_ENABLED
#define AFL_STATIC_ASSERT(e, ...) static_assert(e, "" __VA_ARGS__)
#else
#define AFL_STATIC_ASSERT(e, ...) AFL_STATIC_ASSERT_IMPL(e, __FILE__, __LINE__)
#define AFL_STATIC_ASSERT_IMPL(e, file, line)                                                      \
    typedef char static_assert_fail_on_##m_file##line[2 * ((e) != 0) - 1]
#endif

} // namespace base
} // namespace afl
