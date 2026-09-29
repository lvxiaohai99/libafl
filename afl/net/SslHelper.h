/**
 * @file   SslHelper.h
 * @brief  OpenSSL 客户端辅助（可选，需 AFL_ENABLE_SSL）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <cstdint>
#include <string>
#include <vector>

namespace afl
{
namespace net
{
/** @brief 一次 SSL 会话句柄 */
struct SslConnection
{
    int socket = -1;
    SSL* sslHandle = nullptr;
    SSL_CTX* sslCtx = nullptr;
};

/** @brief 证书路径三元组 */
struct SslCertPaths
{
    std::string caCertificateFile;
    std::string clientCertificateFile;
    std::string clientPrivateKeyFile;
};

/**
 * @brief 轻量 SSL 客户端助手（不 abort / 不 exit）
 *
 * 设计说明：旧实现用 exit()/CHK_* 宏，不适合库代码；本类改为返回 false。
 */
class SslHelper
{
public:
    SslHelper();
    ~SslHelper();

    /** @brief 初始化 OpenSSL 库（进程内可多次调用） */
    void initSsl();

    /**
     * @brief 释放会话并关闭 fd
     * @param conn 会话
     * @param fd   socket fd
     */
    void deinitSsl(SslConnection* conn, uint32_t fd);

    /**
     * @brief 创建双向认证上下文
     * @param conn 输出会话（填充 sslCtx）
     * @param cert 证书路径
     * @return 成功 true；证书缺失或 OpenSSL 失败返回 false（不退出进程）
     */
    bool createVerifyContext(SslConnection* conn, const SslCertPaths& cert);

    /**
     * @brief 在已连接 fd 上做 SSL 握手
     * @param conn 已设置 sslCtx 的会话
     * @param fd   已 connect 的 socket
     * @return 握手成功 true
     */
    bool doSslConnect(SslConnection* conn, uint32_t fd);

    /**
     * @brief 打印对端证书摘要（调试用）
     * @param conn 已握手会话
     * @return 有证书返回 true
     */
    bool showCerts(SslConnection* conn);

    /**
     * @brief SSL 写
     * @param conn 会话
     * @param data 数据
     * @param len  长度
     * @return 已写字节数；失败返回 -1
     */
    int sslSend(SslConnection* conn, const char* data, int len);

    /**
     * @brief SSL 读
     * @param conn   会话
     * @param out    输出缓冲
     * @param maxLen 最大读取
     * @return 已读字节数；失败/关闭返回 <=0
     */
    int sslRecv(SslConnection* conn, char* out, int maxLen);
};

} // namespace net
} // namespace afl
