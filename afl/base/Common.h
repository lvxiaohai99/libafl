/**
 * @file   Common.h
 * @brief  libafl 公共头文件：平台宏、通用工具宏与常量定义。
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>

#define __STDC_FORMAT_MACROS
#include <inttypes.h> ///< printf("%" PRId64 "\n", (int64_t)value);
#undef __STDC_FORMAT_MACROS

/** 库统一运行平台：Linux */
#define OS_LINUX

using std::list;
using std::map;
using std::multimap;
using std::multiset;
using std::queue;
using std::set;
using std::string;
using std::vector;

/** C++11 编译器探测 */
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus > 199711L || __cplusplus == 201103L
#define AFL_CXX11_ENABLED 1
#endif

/** printf 风格格式化（Linux 下即 snprintf） */
#define AFL_SNPRINTF snprintf

/** 忽略未使用变量/参数的告警 */
#define AFL_UNUSED(statement) ((void)(statement))

/** 安全释放单个堆对象指针 */
#define SAFE_DELETE(p)                                                                             \
    do                                                                                             \
    {                                                                                              \
        delete p;                                                                                  \
        p = NULL;                                                                                  \
    } while (0)

/** 安全释放堆数组指针 */
#define SAFE_DELETE_ARRAY(p)                                                                       \
    do                                                                                             \
    {                                                                                              \
        delete[] p;                                                                                \
        p = NULL;                                                                                  \
    } while (0)

/** ֹ븳ֵC++11 delete 汾 */
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                         \
    TypeName(const TypeName&) = delete;                                                            \
    TypeName& operator=(const TypeName&) = delete

/** 异常相关宏（默认启用 try/catch 展开） */
#define USE_TRY_CATCH
#ifdef USE_TRY_CATCH
#define AFL_TRY_BEGIN                                                                              \
    try                                                                                            \
    {
#define AFL_CATCH(x)                                                                               \
    }                                                                                              \
    catch (x)                                                                                      \
    {
#define AFL_CATCH_ALL                                                                              \
    }                                                                                              \
    catch (...)                                                                                    \
    {
#define AFL_CATCH_END }
#define AFL_RAISE(x) throw(x)
#define AFL_RERAISE throw
#define AFL_THROWS(x) throw(x)
#else // USE_TRY_CATCH
#define AFL_TRY_BEGIN                                                                              \
    {                                                                                              \
        {
#define AFL_CATCH(x)                                                                               \
    }                                                                                              \
    if (0)                                                                                         \
    {
#define AFL_CATCH_ALL                                                                              \
    }                                                                                              \
    if (0)                                                                                         \
    {
#define AFL_CATCH_END                                                                              \
    }                                                                                              \
    }
#define AFL_RAISE(x)
#define AFL_RERAISE
#define AFL_THROWS(x)
#endif // USE_TRY_CATCH
