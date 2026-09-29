/**
 * @file   InetAddress.cpp
 * @brief  网络地址封装的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/InetAddress.h"

#include <string.h>
namespace afl
{
namespace net
{
InetAddress::InetAddress(uint16_t port /* = 0*/)
{
    ::memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = INADDR_ANY;
}

InetAddress::InetAddress(const char* ip, uint16_t port)
{
    ::memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    int nIP = 0;
    if (!ip || '\0' == *ip || 0 == strcmp(ip, "0") || 0 == strcmp(ip, "0.0.0.0") ||
        0 == strcmp(ip, "*"))
    {
        nIP = htonl(INADDR_ANY);
    }
    else
    {
        nIP = inet_addr(ip);
    }
    m_addr.sin_addr.s_addr = nIP;
}

InetAddress::InetAddress(const AFL_SOCKADDR_IN& addr) : m_addr(addr) {}

std::string InetAddress::ip() const
{
    char ip[256], tmp[256];
    AFL_SNPRINTF(ip, 128, "%s", inet_ntop(AF_INET, (void*)&m_addr.sin_addr, tmp, 256));
    return ip;
}

uint16_t InetAddress::port() const
{
    return ntohs(m_addr.sin_port);
}

std::string InetAddress::ipPort() const
{
    char host[256], ip[256];
    AFL_SNPRINTF(host, 256, "%s:%d", inet_ntop(AF_INET, (void*)&m_addr.sin_addr, ip, 256),
                 ntohs(m_addr.sin_port));
    return host;
}

/*static*/ bool InetAddress::resolve(const char* hostname, InetAddress* addr)
{
    static __thread char g_resolveBuffer[64 * 1024];
    struct hostent hent;
    struct hostent* he = NULL;
    int herrno = 0;
    bzero(&hent, sizeof(hent));

    int ret =
        gethostbyname_r(hostname, &hent, g_resolveBuffer, sizeof(g_resolveBuffer), &he, &herrno);
    if (ret == 0 && he != NULL)
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        addr->m_addr.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace net
} // namespace afl
