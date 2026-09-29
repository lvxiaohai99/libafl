/**
 * @file   Base64.h
 * @brief  Base64 编解码
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"
#include <string>

namespace afl
{
namespace crypto
{
/**
 * @brief Base64 编码
 * @param src 源数据
 * @param len 源数据长度（字节）
 * @param dst 输出缓冲区（容量至少 len*4/3）
 * @return 编码后长度
 */
size_t base64Encode(const char* src, size_t len, char* dst);
size_t base64Encode(const char* src, size_t len, std::string& dst);
size_t base64Encode(const std::string& src, std::string& dst);
std::string base64Encode(const char* src, size_t len);
std::string base64Encode(const std::string& src);

/**
 * @brief Base64 解码
 * @param src 输入的 Base64 字符串
 * @param len 输入长度（字节）
 * @param dst 输出缓冲区（容量至少 len*3/4）
 * @return 解码后长度
 */
size_t base64Decode(const char* src, size_t len, char* dst);
size_t base64Decode(const char* src, size_t len, std::string& dst);
size_t base64Decode(const std::string& src, std::string& dst);
std::string base64Decode(const char* src, size_t len);
std::string base64Decode(const std::string& src);

} // namespace crypto
} // namespace afl
