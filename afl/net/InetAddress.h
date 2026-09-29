/**
 * @file   InetAddress.h
 * @brief  网络地址（IP:port）封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/net/SocketUtil.h"

#include <netinet/in.h>
#include <arpa/inet.h>

namespace afl
{
namespace net
{
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0);
    InetAddress(const char* ip, uint16_t port);
    InetAddress(const AFL_SOCKADDR_IN& addr);

    static bool resolve(const char* hostname, InetAddress* addr);

public:
    uint16_t port() const;
    std::string ip() const;
    std::string ipPort() const;

    size_t addressLength() const { return sizeof(m_addr); }
    operator struct sockaddr *() const { return (struct sockaddr*)&m_addr; }

    const AFL_SOCKADDR_IN& getSockAddrInet() const { return m_addr; }
    void setSockAddrInet(const AFL_SOCKADDR_IN& addr) { m_addr = addr; }

    uint32_t ipNetEndian() const { return m_addr.sin_addr.s_addr; }
    uint16_t portNetEndian() const { return m_addr.sin_port; }

private:
    AFL_SOCKADDR_IN m_addr;
};

} // namespace net
} // namespace afl
