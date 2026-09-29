/**
 * @file   AesHelper.h
 * @brief  AES-ECB（PKCS5Padding，128-bit）加解密辅助类
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include <string>

namespace afl
{
namespace crypto
{
/** AES 块长度（字节） */
#define AFL_AES_BLOCK_SIZE 16

/**
 * @brief AES-ECB 加解密辅助（可选 Base64 包装）
 */
class AesHelper
{
public:
    AesHelper() = default;
    ~AesHelper() = default;

    /**
     * @brief 加密后做 Base64 编码
     * @param src 明文
     * @param key 密钥（不足 16 字节按 PKCS5 填充）
     * @return Base64 密文
     */
    static std::string encrypt(std::string src, std::string key);

    /**
     * @brief Base64 解码后再解密
     * @param src Base64 密文
     * @param key 密钥
     * @return 明文
     */
    static std::string decrypt(std::string src, std::string key);

    /**
     * @brief 加密（原始二进制，不做 Base64）
     * @param src 明文
     * @param key 密钥
     * @return 二进制密文
     */
    static std::string encryptNoBase64(std::string src, std::string key);

    /**
     * @brief 解密原始二进制密文
     * @param src 二进制密文
     * @param key 密钥
     * @return 明文
     */
    static std::string decryptNoBase64(std::string src, std::string key);

private:
    static void stringToHex(const char* src, unsigned char* dest, int srcLen, int destLen);
    static void padding(unsigned char* src, int srcLen);
};

} // namespace crypto
} // namespace afl
