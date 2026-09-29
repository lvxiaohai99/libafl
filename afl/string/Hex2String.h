/**
 * @file   Hex2String.h
 * @brief  十六进制与字符串转换工具
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <string>

namespace afl
{
namespace str
{
const static char g_hexNum[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

const static char g_hexStr[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                '9', 'A', 'B', 'C', 'D', 'E', 'F', '\0'};

inline std::string decodeHexString(const std::string& hexstr)
{
    std::string buf;
    int index = 0;
    while (hexstr[index] != '\0')
    {
        buf += static_cast<char>(g_hexNum[hexstr[index]] * 16 + g_hexNum[hexstr[index + 1]]);
        index += 2;
    }
    return buf;
}

inline std::string encodeToHexString(const std::string& str)
{
    std::string hexbuf;
    int index = 0;
    while (str[index] != '\0')
    {
        hexbuf += g_hexStr[static_cast<unsigned char>(str[index]) / 16];
        hexbuf += g_hexStr[static_cast<unsigned char>(str[index]) % 16];
        ++index;
    }
    return hexbuf;
}

/**
 * 二进制安全：按字节输出大写 hex，字节间以空格分隔（可含 0x00）。
 * @param bytesPerLine 每行字节数，0 表示不换行
 * @return 例如 "30 0A 1F\n00 FF"
 */
inline std::string toHexDump(const std::string& bytes, size_t bytesPerLine = 16)
{
    std::string out;
    out.reserve(bytes.size() * 3);
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (i > 0)
        {
            out += (bytesPerLine > 0 && i % bytesPerLine == 0) ? '\n' : ' ';
        }
        const unsigned char b = static_cast<unsigned char>(bytes[i]);
        out += g_hexStr[b >> 4];
        out += g_hexStr[b & 0x0F];
    }
    return out;
}

/**
 * 二进制安全、宽松解析 hex 文本为字节。
 * 接受大小写、空白/换行、分隔符 ':' ',' '-'，以及每个字节前可选的 "0x"/"0X"，
 * 如 "30 0a"、"300A"、"0x30,0x0A"、"30:0A"。
 * @return 含非法字符或 hex 位数为奇数时返回 false，out 不保证内容
 */
inline bool parseHexBytes(const std::string& text, std::string& out)
{
    out.clear();
    int high = -1;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ':' || c == ',' || c == '-')
        {
            continue;
        }
        if (c == '0' && high < 0 && i + 1 < text.size() && (text[i + 1] == 'x' || text[i + 1] == 'X'))
        {
            ++i;
            continue;
        }
        int v;
        if (c >= '0' && c <= '9')
        {
            v = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            v = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            v = c - 'A' + 10;
        }
        else
        {
            return false;
        }
        if (high < 0)
        {
            high = v;
        }
        else
        {
            out += static_cast<char>((high << 4) | v);
            high = -1;
        }
    }
    return high < 0;
}
} // namespace str
} // namespace afl
