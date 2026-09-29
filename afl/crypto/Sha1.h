/**
 * @file   Sha1.h
 * @brief  SHA1 安全哈希算法
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <string>
#include <stdint.h>
namespace afl
{
namespace crypto
{
// 对于长度小于 2^64 位的消息，SHA1 会产生一个 160 位的消息摘要。当接收到消息时，该摘要可用于验证数据的完整性。
// 在传输过程中数据很可能发生变化，此时就会产生不同的信息摘要。
// SHA1 特性：不可以从消息摘要中复原信息；两个不同的消息不会产生同样的消息摘要。

class SHA1
{
public:
    SHA1();
    ~SHA1();

    static std::string hexDigest(const std::string& src);

public:
    void reset();
    void update(const std::string& sp);
    /** @brief 结束 SHA1 运算并将 20 字节摘要写入 digest */
    void final(void* digest);
    /** @brief 返回十六进制编码的摘要字符串 */
    std::string hexFinal();

private:
    void sha1Transform(uint32_t state[5], const uint8_t buffer[64]);
    void update(const uint8_t* data, size_t input_len);
    void finalInternal();

private:
    SHA1(const SHA1&);
    const SHA1& operator=(const SHA1&);

    struct SHA1_CTX
    {
        uint32_t state[5];
        uint32_t count[2]; // Bit count of input.
        uint8_t buffer[64];
    };

    SHA1_CTX m_context;
    uint8_t m_digest[20];
};

} // namespace crypto
} // namespace afl
