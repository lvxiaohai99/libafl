/**
 * @file   StringHash.h
 * @brief  字符串哈希函数
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
namespace afl
{
namespace crypto
{
/** @brief SDBM 字符串哈希 */
unsigned int SDBMHash(const char* str);

/** @brief RS 字符串哈希 */
unsigned int RSHash(const char* str);

/** @brief JS 字符串哈希 */
unsigned int JSHash(const char* str);

/** @brief BKDR 字符串哈希 */
unsigned int BKDRHash(const char* str);

/** @brief DJB 字符串哈希 */
unsigned int DJBHash(const char* str);

/** @brief FNV 字符串哈希 */
unsigned int FNVHash(const char* str);

} // namespace crypto
} // namespace afl
