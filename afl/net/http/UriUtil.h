/**
 * @file   UriUtil.h
 * @brief  URI 解析工具
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <string>
namespace afl
{
namespace net
{
/** @brief URI 编码到外部缓冲，返回写入长度 */
size_t uriEncode(const char* unencoded, size_t len, char* encoded);
/** @brief URI 编码为 string */
std::string uriEncode(const char* unencoded, size_t len);
std::string uriEncode(const std::string& unencoded);

/** @brief URI 解码到外部缓冲，返回写入长度 */
size_t uriDecode(const char* encoded, size_t len, char* dst);
/** @brief URI 解码为 string */
std::string uriDecode(const char* encoded, size_t len);
std::string uriDecode(const std::string& encoded);

} // namespace net
} // namespace afl
