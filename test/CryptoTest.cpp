/**
 * @file   CryptoTest.cpp
 * @brief  crypto 模块单元测试：Base64 / Md5 / Sha1 / Crc / AesHelper
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <string>

#include "afl/crypto/Base64.h"
#include "afl/crypto/Md5.h"
#include "afl/crypto/Sha1.h"
#include "afl/crypto/Crc.h"
#include "afl/crypto/AesHelper.h"
#include "afl/crypto/StringHash.h"

using namespace afl::crypto;

TEST(CryptoTest, Base64KnownVector)
{
    EXPECT_EQ("aGVsbG8=", base64Encode("hello"));
    EXPECT_EQ("hello", base64Decode("aGVsbG8="));
    EXPECT_EQ("", base64Encode(std::string()));
    EXPECT_EQ("", base64Decode(std::string()));
}

TEST(CryptoTest, Base64RoundTrip)
{
    const char* cases[] = {
        "libafl",
        "a",
        "ab",
        "abc",
        "abcd",
        "123456789012345",
        "\x01\x02\x03\xff\xfe",
        "The quick brown fox jumps over the lazy dog",
        "中文内容 test 123",
    };
    for (const char* raw : cases)
    {
        std::string enc = base64Encode(raw);
        std::string dec = base64Decode(enc);
        EXPECT_EQ(std::string(raw), dec);
    }
}

TEST(CryptoTest, Md5KnownVector)
{
    EXPECT_EQ("900150983cd24fb0d6963f7d28e17f72", MD5("abc").hexdigest());
    EXPECT_EQ("d41d8cd98f00b204e9800998ecf8427e", MD5("").hexdigest());
    EXPECT_EQ("c4ca4238a0b923820dcc509a6f75849b", MD5("1").md5());
}

TEST(CryptoTest, Sha1KnownVector)
{
    EXPECT_EQ("a9993e364706816aba3e25717850c26c9cd0d89d", SHA1::hexDigest("abc"));
    EXPECT_EQ("da39a3ee5e6b4b0d3255bfef95601890afd80709", SHA1::hexDigest(""));
}

TEST(CryptoTest, CrcStable)
{
    const std::string data = "123456789";
    uint32_t c32a = CRC32::calc(const_cast<char*>(data.data()), static_cast<int>(data.size()));
    uint32_t c32b = CRC32::calc(const_cast<char*>(data.data()), static_cast<int>(data.size()));
    EXPECT_EQ(c32a, c32b);
    EXPECT_NE(c32a, 0u);

    uint16_t c16 = CRC16::calc(const_cast<char*>(data.data()), static_cast<int>(data.size()));
    uint8_t c8 = CRC8::calc(const_cast<char*>(data.data()), static_cast<int>(data.size()));
    (void)c16;
    (void)c8;
    SUCCEED();
}

TEST(CryptoTest, AesHelperRoundTrip)
{
    const std::string key = "0123456789abcdef";
    const std::string plain = "libafl aes ecb test!";

    std::string cipher = AesHelper::encryptNoBase64(plain, key);
    EXPECT_FALSE(cipher.empty());
    EXPECT_EQ(0u, cipher.size() % 16);

    std::string recovered = AesHelper::decryptNoBase64(cipher, key);
    EXPECT_EQ(plain, recovered);
}

TEST(CryptoTest, AesHelperBase64RoundTrip)
{
    const std::string key = "0123456789abcdef";
    const std::string plain = "hello libafl";

    std::string cipher = AesHelper::encrypt(plain, key);
    EXPECT_FALSE(cipher.empty());

    std::string recovered = AesHelper::decrypt(cipher, key);
    EXPECT_EQ(plain, recovered);
}

TEST(CryptoTest, StringHashStableAndDistinct)
{
    const char* s = "libafl";
    EXPECT_EQ(SDBMHash(s), SDBMHash(s));
    EXPECT_EQ(BKDRHash(s), BKDRHash(s));
    EXPECT_NE(DJBHash(s), DJBHash("libafL"));
    EXPECT_NE(0u, FNVHash(s));
}
