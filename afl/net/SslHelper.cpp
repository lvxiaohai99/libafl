/**
 * @file   SslHelper.cpp
 * @brief  OpenSSL 辅助实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/SslHelper.h"

#include "afl/file/FileUtil.h"

#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace afl
{
namespace net
{
SslHelper::SslHelper() {}
SslHelper::~SslHelper() {}

void SslHelper::initSsl()
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
}

void SslHelper::deinitSsl(SslConnection* conn, uint32_t fd)
{
    if (!conn)
    {
        return;
    }
    if (conn->sslHandle)
    {
        SSL_shutdown(conn->sslHandle);
        SSL_free(conn->sslHandle);
        conn->sslHandle = nullptr;
    }
    if (conn->sslCtx)
    {
        SSL_CTX_free(conn->sslCtx);
        conn->sslCtx = nullptr;
    }
    if (fd != static_cast<uint32_t>(-1))
    {
        ::shutdown(static_cast<int>(fd), SHUT_RDWR);
    }
}

bool SslHelper::createVerifyContext(SslConnection* conn, const SslCertPaths& cert)
{
    if (!conn)
    {
        return false;
    }
    conn->sslCtx = nullptr;
    conn->sslHandle = nullptr;

    if (!afl::file::FileUtil::isFileExist(cert.caCertificateFile.c_str()) ||
        !afl::file::FileUtil::isFileExist(cert.clientCertificateFile.c_str()) ||
        !afl::file::FileUtil::isFileExist(cert.clientPrivateKeyFile.c_str()))
    {
        return false;
    }

    conn->sslCtx = SSL_CTX_new(SSLv23_method());
    if (!conn->sslCtx)
    {
        return false;
    }

    SSL_CTX_set_verify(conn->sslCtx, SSL_VERIFY_NONE, nullptr);

    if (!SSL_CTX_load_verify_locations(conn->sslCtx, cert.caCertificateFile.c_str(), nullptr))
    {
        SSL_CTX_free(conn->sslCtx);
        conn->sslCtx = nullptr;
        return false;
    }
    if (SSL_CTX_use_certificate_file(conn->sslCtx, cert.clientCertificateFile.c_str(),
                                     SSL_FILETYPE_PEM) <= 0)
    {
        SSL_CTX_free(conn->sslCtx);
        conn->sslCtx = nullptr;
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(conn->sslCtx, cert.clientPrivateKeyFile.c_str(),
                                    SSL_FILETYPE_PEM) <= 0)
    {
        SSL_CTX_free(conn->sslCtx);
        conn->sslCtx = nullptr;
        return false;
    }
    if (!SSL_CTX_check_private_key(conn->sslCtx))
    {
        SSL_CTX_free(conn->sslCtx);
        conn->sslCtx = nullptr;
        return false;
    }
    return true;
}

bool SslHelper::doSslConnect(SslConnection* conn, uint32_t fd)
{
    if (!conn || !conn->sslCtx)
    {
        return false;
    }

    conn->sslHandle = SSL_new(conn->sslCtx);
    if (!conn->sslHandle)
    {
        return false;
    }

    SSL_set_fd(conn->sslHandle, static_cast<int>(fd));
    SSL_set_connect_state(conn->sslHandle);

    for (;;)
    {
        const int rc = SSL_connect(conn->sslHandle);
        if (rc == 1)
        {
            return true;
        }
        const int err = SSL_get_error(conn->sslHandle, rc);
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
        {
            continue;
        }
        SSL_free(conn->sslHandle);
        conn->sslHandle = nullptr;
        return false;
    }
}

bool SslHelper::showCerts(SslConnection* conn)
{
    if (!conn || !conn->sslHandle)
    {
        return false;
    }

    X509* serverCert = SSL_get_peer_certificate(conn->sslHandle);
    if (!serverCert)
    {
        return false;
    }
    char* subject = X509_NAME_oneline(X509_get_subject_name(serverCert), nullptr, 0);
    char* issuer = X509_NAME_oneline(X509_get_issuer_name(serverCert), nullptr, 0);
    if (subject)
    {
        OPENSSL_free(subject);
    }
    if (issuer)
    {
        OPENSSL_free(issuer);
    }
    X509_free(serverCert);
    return true;
}

int SslHelper::sslSend(SslConnection* conn, const char* data, int len)
{
    if (!conn || !conn->sslHandle || !data || len <= 0)
    {
        return -1;
    }

    int written = 0;
    while (written < len)
    {
        const int n = SSL_write(conn->sslHandle, data + written, len - written);
        if (n > 0)
        {
            written += n;
            continue;
        }
        const int err = SSL_get_error(conn->sslHandle, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        {
            continue;
        }
        return written > 0 ? written : -1;
    }
    return written;
}

int SslHelper::sslRecv(SslConnection* conn, char* out, int maxLen)
{
    if (!conn || !conn->sslHandle || !out || maxLen <= 0)
    {
        return -1;
    }

    for (;;)
    {
        const int n = SSL_read(conn->sslHandle, out, maxLen);
        if (n > 0)
        {
            return n;
        }
        const int err = SSL_get_error(conn->sslHandle, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        {
            continue;
        }
        return n;
    }
}

} // namespace net
} // namespace afl
