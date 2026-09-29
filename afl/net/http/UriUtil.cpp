/**
 * @file   UriUtil.cpp
 * @brief  URI 解析工具的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/http/UriUtil.h"
namespace afl
{
namespace net
{
// 百分号编码参考 Wikipedia Percent-encoding
static inline bool is_unreserved_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
           c == '_' || c == '.' || c == '~';
}

/** @brief 单个十六进制字符转数值（假定输入合法） */
static inline char char_hex_to_numeric(char hex)
{
    if (hex >= 'a' && hex <= 'f')
    {
        return hex - 'a' + 10;
    }

    if (hex >= 'A' && hex <= 'F')
    {
        return hex - 'A' + 10;
    }

    return hex - '0';
}

/** @brief 两个十六进制字符转数值；非法输入返回 0 */
static inline char two_char_hex_to_numeric(char hex_high, char hex_low)
{
    if (!isxdigit(hex_high) || !isxdigit(hex_low))
    {
        return 0;
    }
    return char_hex_to_numeric(hex_high) * 16 + char_hex_to_numeric(hex_low);
}

static inline unsigned char charToHex(unsigned char x)
{
    return x > 9 ? x - 10 + 'A' : x + '0';
}

static inline unsigned char hexToChar(unsigned char x)
{
    unsigned char y = 0;
    if (x >= 'A' && x <= 'Z')
        y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
        y = x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        y = x - '0';
    else
        y = 0;
    return y;
}

size_t uriEncode(const char* unencoded, size_t len, char* encoded)
{
    size_t j = 0;
    for (size_t i = 0; i < len; i++)
    {
        char c = unencoded[i];
        if (is_unreserved_char(c))
        {
            encoded[j++] = c;
        }
        else
        {
            encoded[j++] += '%';
            encoded[j++] += charToHex((unsigned char)c >> 4);
            encoded[j++] += charToHex((unsigned char)c % 16);
        }
    }
    return j;
}

std::string uriEncode(const char* unencoded, size_t len)
{
    std::string encoded;

    for (size_t i = 0; i < len; i++)
    {
        char c = unencoded[i];
        if (is_unreserved_char(c))
        {
            encoded += c;
        }
        else
        {
            encoded += '%';
            encoded += charToHex((unsigned char)c >> 4);
            encoded += charToHex((unsigned char)c % 16);
        }
    }

    return encoded;
}

std::string uriEncode(const std::string& uri)
{
    return uriEncode(uri.c_str(), uri.size());
}

size_t uriDecode(const char* encoded, size_t len, char* dst)
{
    size_t j = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (encoded[i] == '+')
        {
            dst[j++] = ' ';
        }
        else if (encoded[i] == '%')
        {
            if (i + 2 < len)
            {
                dst[j++] = two_char_hex_to_numeric(encoded[i + 1], encoded[i + 2]);
            }
            i += 2;
        }
        else
        {
            dst[j++] = encoded[i];
        }
    }
    return j;
}

std::string uriDecode(const char* encoded, size_t len)
{
    std::string unencoded("");
    for (size_t i = 0; i < len; i++)
    {
        char c = encoded[i];
        if (c == '+')
        {
            unencoded += ' ';
        }
        else if (c == '%')
        {
            if (i + 2 < len)
            {
                char unencoded_char = two_char_hex_to_numeric(encoded[i + 1], encoded[i + 2]);
                if (unencoded_char != 0) // 0 表示非法十六进制
                {
                    unencoded += unencoded_char;
                }
            }
            i += 2;
        }
        else
        {
            unencoded += c;
        }
    }
    return unencoded;
}

std::string uriDecode(const std::string& encoded)
{
    return uriDecode(encoded.c_str(), encoded.size());
    //std::string unencoded;
    //for (int i = 0; i < encoded.length(); i++) {
    //    char c = encoded[i];
    //    if (c == '%') {
    //        if (i + 2 < encoded.length()) {
    //            char unencoded_char = two_char_hex_to_numeric(encoded[i + 1], encoded[i + 2]);
    //            if (unencoded_char != 0) {  // 0 signals bad (non-hex) input.
    //                unencoded += unencoded_char;
    //            }
    //        }
    //        i += 2;
    //    }
    //    else if (c == '+') {
    //        unencoded += ' ';
    //    }
    //    else {
    //        unencoded += c;
    //    }
    //}
    //return unencoded;
}

} // namespace net
} // namespace afl
