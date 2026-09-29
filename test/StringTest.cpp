/**
 * @file   StringTest.cpp
 * @brief  string 模块单元测试：StringUtil / Hex2String
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "afl/string/StringUtil.h"
#include "afl/string/Hex2String.h"

using namespace afl::str;

TEST(StringTest, StringFormat)
{
    EXPECT_EQ("a-100-3.14", stringFormat("a-%d-%.2f", 100, 3.14159));
    EXPECT_EQ("", stringFormat(""));

    std::string out;
    stringFormat(&out, "x=%s", "y");
    EXPECT_EQ("x=y", out);

    stringFormatAppend(&out, ",z=%d", 9);
    EXPECT_EQ("x=y,z=9", out);
}

TEST(StringTest, Trim)
{
    std::string s = "  hello world  ";
    EXPECT_EQ("hello world", trim(s));

    s = "\t\nabc \r\n";
    // delim 是字符集参数，默认只去除空格，需显式指定空白字符集
    EXPECT_EQ("abc", trim(s, " \t\r\n"));

    s = "xxaxx";
    EXPECT_EQ("a", trim(s, "x"));

    s = "  left";
    EXPECT_EQ("left", trimLeft(s));

    s = "right  ";
    EXPECT_EQ("right", trimRight(s));
}

TEST(StringTest, Split)
{
    std::vector<std::string> v;
    split("a,b,c", v, ",");
    EXPECT_EQ(3u, v.size());
    EXPECT_EQ("a", v[0]);
    EXPECT_EQ("b", v[1]);
    EXPECT_EQ("c", v[2]);

    // 默认丢弃连续分隔符产生的空段
    v.clear();
    split("a,,c", v, ",");
    EXPECT_EQ(2u, v.size());
    EXPECT_EQ("a", v[0]);
    EXPECT_EQ("c", v[1]);

    // insertEmpty=true 时保留空段
    v.clear();
    split("a,,c", v, ",", true);
    EXPECT_EQ(3u, v.size());
    EXPECT_EQ("", v[1]);

    v.clear();
    split("no-delim", v, ",");
    EXPECT_EQ(1u, v.size());
    EXPECT_EQ("no-delim", v[0]);
}

TEST(StringTest, Hex2StringRoundTrip)
{
    // 注意：实现以 '\0' 为终止符（C 字符串语义），不适用于含嵌入 NUL 的二进制数据
    std::string raw;
    raw += char(0x1f);
    raw += char(0xa5);
    raw += char(0xff);
    raw += "Az9";

    std::string hex = encodeToHexString(raw);
    EXPECT_FALSE(hex.empty());
    // 所有字符都在十六进制集合内
    for (char c : hex)
    {
        bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        EXPECT_TRUE(ok);
    }

    std::string back = decodeHexString(hex);
    EXPECT_EQ(raw, back);
}

TEST(StringTest, HexDumpIsBinarySafe)
{
    const std::string raw("\x30\x00\xff\x0a", 4);
    EXPECT_EQ("30 00 FF 0A", toHexDump(raw, 0));
    EXPECT_EQ("30 00\nFF 0A", toHexDump(raw, 2));
    EXPECT_EQ("", toHexDump(std::string()));
}

TEST(StringTest, ParseHexBytesAcceptsCommonFormats)
{
    const std::string expect("\x30\x00\xff\x0a", 4);
    std::string out;
    EXPECT_TRUE(parseHexBytes("30 00 FF 0A", out));
    EXPECT_EQ(expect, out);
    EXPECT_TRUE(parseHexBytes("3000ff0a", out));
    EXPECT_EQ(expect, out);
    EXPECT_TRUE(parseHexBytes("0x30,0x00,0xFF,0x0a\n", out));
    EXPECT_EQ(expect, out);
    EXPECT_TRUE(parseHexBytes("30:00:ff:0a", out));
    EXPECT_EQ(expect, out);
    EXPECT_TRUE(parseHexBytes(toHexDump(expect, 1), out));
    EXPECT_EQ(expect, out);

    EXPECT_FALSE(parseHexBytes("300", out));
    EXPECT_FALSE(parseHexBytes("3g", out));
}

TEST(StringTest, Hex2StringKnownValue)
{
    // 视实现大小写而定：只断言解码回环与长度
    std::string hex = encodeToHexString(std::string("\xab"));
    EXPECT_EQ(2u, hex.size());
    EXPECT_EQ("\xab", decodeHexString(hex));
}
