/**
 * @file   Aes.h
 * @brief  AES 加解密算法
 * @author libafl
 * @date   2026-09
 */
#pragma once
#ifndef uint8
#define uint8 unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif

typedef struct
{
    uint32 erk[64]; /* 加密轮密钥 */
    uint32 drk[64]; /* 解密轮密钥 */
    int nr;         /* AES 轮数 */
} aes_context;

namespace afl
{
namespace crypto
{
int aesSetKey(aes_context* ctx, uint8* key, int nbits);
void aesEncrypt(aes_context* ctx, uint8 input[16], uint8 output[16]);
void aesDecrypt(aes_context* ctx, uint8 input[16], uint8 output[16]);

} // namespace crypto
} // namespace afl
