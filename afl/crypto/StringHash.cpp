/**
 * @file   StringHash.cpp
 * @brief  字符串哈希函数的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/crypto/StringHash.h"
namespace afl
{
namespace crypto
{
/** SDBM 哈希 */
unsigned int SDBMHash(const char* str)
{
    unsigned int hash = 0;
    while (*str)
    {
        // 等价于 hash = 65599*hash + (*str++)
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    }

    return (hash & 0x7FFFFFFF);
}

/** RS 哈希 */
unsigned int RSHash(const char* str)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * a + (*str++);
        a *= b;
    }

    return (hash & 0x7FFFFFFF);
}

/** JS 哈希 */
unsigned int JSHash(const char* str)
{
    unsigned int hash = 1315423911;

    while (*str)
    {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }

    return (hash & 0x7FFFFFFF);
}

/** BKDR 哈希 */
unsigned int BKDRHash(const char* str)
{
    // 常用 seed：131 等
    unsigned int hash = 0;

    while (*str)
    {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }

    return (hash & 0x7FFFFFFF);
}

/** DJB 哈希 */
unsigned int DJBHash(const char* str)
{
    unsigned int hash = 5381;

    while (*str)
    {
        hash += (hash << 5) + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

unsigned int FNVHash(const char* str)
{
    register unsigned int hash = 2166136261;
    while (unsigned int ch = (unsigned int)*str++)
    {
        hash *= 16777619;
        hash ^= ch;
    }
    return hash;
}

} // namespace crypto
} // namespace afl
