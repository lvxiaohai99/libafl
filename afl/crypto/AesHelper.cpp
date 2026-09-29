/**
 * @file   AesHelper.cpp
 * @brief  AES-ECB 加解密辅助实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/crypto/AesHelper.h"
#include "afl/crypto/Aes.h"
#include "afl/crypto/Base64.h"

#include <cstring>
#include <string>

namespace afl
{
namespace crypto
{
void AesHelper::stringToHex(const char* src, unsigned char* dest, int srcLen, int destLen)
{
    if (src != nullptr)
    {
        memcpy(dest, src, srcLen > destLen ? destLen : srcLen);
    }
    padding(dest, srcLen > destLen ? destLen : srcLen);
}

void AesHelper::padding(unsigned char* src, int srcLen)
{
    if (srcLen < AFL_AES_BLOCK_SIZE)
    {
        unsigned char pad = static_cast<unsigned char>(AFL_AES_BLOCK_SIZE - srcLen);
        for (int i = AFL_AES_BLOCK_SIZE; i > srcLen; --i)
        {
            src[i - 1] = pad;
        }
    }
}

std::string AesHelper::encrypt(std::string src, std::string key)
{
    std::string binary = encryptNoBase64(src, key);
    return base64Encode(binary);
}

std::string AesHelper::decrypt(std::string src, std::string key)
{
    std::string binary = base64Decode(src);
    return decryptNoBase64(binary, key);
}

std::string AesHelper::encryptNoBase64(std::string src, std::string key)
{
    aes_context ctx;
    unsigned char buf[AFL_AES_BLOCK_SIZE];
    unsigned char keyBuf[AFL_AES_BLOCK_SIZE];
    stringToHex(key.c_str(), keyBuf, static_cast<int>(key.length()), AFL_AES_BLOCK_SIZE);
    aesSetKey(&ctx, keyBuf, 128);

    const char* pSrc = src.c_str();
    int nSrcLen = static_cast<int>(src.size());
    int nDestLen = (nSrcLen / AFL_AES_BLOCK_SIZE) * AFL_AES_BLOCK_SIZE + AFL_AES_BLOCK_SIZE;
    unsigned char* pDest = new unsigned char[nDestLen];
    memset(pDest, 0, nDestLen);

    const char* pTmpSrc = pSrc;
    unsigned char* pTmpDest = pDest;
    int tmpSrcLen = nSrcLen;
    while ((pTmpSrc - pSrc) < nSrcLen)
    {
        stringToHex(pTmpSrc, buf, tmpSrcLen, AFL_AES_BLOCK_SIZE);
        aesEncrypt(&ctx, buf, buf);
        memcpy(pTmpDest, buf, AFL_AES_BLOCK_SIZE);
        pTmpSrc += AFL_AES_BLOCK_SIZE;
        pTmpDest += AFL_AES_BLOCK_SIZE;
        tmpSrcLen -= AFL_AES_BLOCK_SIZE;
    }
    if ((pTmpDest - pDest) < nDestLen)
    {
        stringToHex(nullptr, buf, 0, AFL_AES_BLOCK_SIZE);
        aesEncrypt(&ctx, buf, buf);
        memcpy(pTmpDest, buf, AFL_AES_BLOCK_SIZE);
    }

    std::string ret;
    ret.assign(reinterpret_cast<char*>(pDest), nDestLen);
    delete[] pDest;
    return ret;
}

std::string AesHelper::decryptNoBase64(std::string src, std::string key)
{
    aes_context ctx;
    unsigned char buf[AFL_AES_BLOCK_SIZE];
    unsigned char keyBuf[AFL_AES_BLOCK_SIZE];
    stringToHex(key.c_str(), keyBuf, static_cast<int>(key.length()), AFL_AES_BLOCK_SIZE);
    aesSetKey(&ctx, keyBuf, 128);

    const char* pSrc = src.data();
    int nSrcLen = static_cast<int>(src.length());
    int nDestLen = nSrcLen;
    unsigned char* pDest = new unsigned char[nDestLen];
    memset(pDest, 0, nDestLen);

    const char* pTmpSrc = pSrc;
    unsigned char* pTmpDest = pDest;
    while ((pTmpSrc - pSrc) < nSrcLen)
    {
        memcpy(buf, pTmpSrc, AFL_AES_BLOCK_SIZE);
        aesDecrypt(&ctx, buf, buf);
        memcpy(pTmpDest, buf, AFL_AES_BLOCK_SIZE);
        pTmpSrc += AFL_AES_BLOCK_SIZE;
        pTmpDest += AFL_AES_BLOCK_SIZE;
    }

    unsigned char ucTest = 0;
    while ((ucTest = *(pTmpDest - 1)))
    {
        if (ucTest > 0 && ucTest <= 0x10)
        {
            *(pTmpDest - 1) = 0;
        }
        else
        {
            break;
        }
        pTmpDest--;
        nDestLen--;
    }

    std::string ret;
    ret.assign(reinterpret_cast<char*>(pDest), nDestLen);
    delete[] pDest;
    return ret;
}

} // namespace crypto
} // namespace afl
