/**
 * @file   DomainResolver.h
 * @brief  域名解析工具
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/InetAddress.h"

struct hostent;
struct addrinfo;
namespace afl
{
namespace net
{
class DomainResolver;

/// 存储主机名别名ip地址
class HostEntry
{
public:
    typedef std::vector<std::string> AliasList;
    typedef std::vector<InetAddress> AddressList;

public:
    void initialize(const struct hostent* entry);

    void initialize(const struct addrinfo* info);

    const std::string& name() const { return m_hostName; }

    const AliasList aliases() const { return m_aliases; }

    const AddressList& addresses() const { return m_addresses; }

    void clear()
    {
        m_hostName.clear();
        m_aliases.clear();
        m_addresses.clear();
    }

    void swap(HostEntry& other)
    {
        std::swap(m_hostName, other.m_hostName);
        std::swap(m_aliases, other.m_aliases);
        std::swap(m_addresses, other.m_addresses);
    }

private:
    void add(const InetAddress& address);

private:
    std::string m_hostName;
    AliasList m_aliases; /// alias names
    AddressList m_addresses;
};

class DomainResolver
{
public:
    static bool query(const std::string& hostname, HostEntry* host, int* error = NULL);

    static bool resolveInetAddress(const std::string& hostname, std::vector<InetAddress>* ips,
                                   int* error = NULL);

    static bool isError(int error);

    static std::string errorString(int error);
};


} // namespace net
} // namespace afl
