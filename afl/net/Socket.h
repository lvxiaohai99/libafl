/**
 * @file   Socket.h
 * @brief  socket 的 RAII 封装，管理 socket fd 生命周期
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/NonCopy.h"
#include "afl/net/SocketUtil.h"

namespace afl
{
namespace net
{
class SocketAddress;
class InetAddress;

class Socket : afl::base::NonCopy
{
public:
    explicit Socket(AFL_SOCKET fd);
    ~Socket();

public:
    // 服务端初始化
    bool bind(const char* ip, int port);
    bool bind(const InetAddress& addr);
    bool listen(int backlog = 5) const;
    AFL_SOCKET accept(AFL_SOCKADDR_IN* peerAddr) const;
    AFL_SOCKET accept(InetAddress* peerAddr) const;
    void close();

    // 客户端初始化
    bool connect(const char* ip, const int port);

    // Socket 选项
    /** @brief 设置/清除非阻塞 */
    bool setNonBlocking(bool on = true);

    /** @brief 设置 TCP_NODELAY（关闭 Nagle） */
    bool setNoDelay(bool on = true);

    /** @brief 设置 SO_REUSEADDR（TIME_WAIT 复用） */
    bool setReuseAddr(bool on = true);

    /** @brief 设置 SO_REUSEPORT */
    bool setReusePort(bool on = true);

    /** @brief 设置 SO_KEEPALIVE */
    bool setKeepAlive(bool on = true);

    /** @brief 设置/读取 SO_SNDBUF */
    bool setSendBuffer(int size);
    bool getSendBuffer(int* size);

    /** @brief 设置/读取 SO_RCVBUF */
    bool setRecvBuffer(int size);
    bool getRecvBuffer(int* size);

    /** @brief 设置/读取 SO_SNDTIMEO */
    bool setSendTimeout(long long timeoutMs);
    bool getSendTimeout(long long* timeoutMs);

    /** @brief 设置/读取 SO_RCVTIMEO */
    bool setRecvTimeout(long long timeoutMs);
    bool getRecvTimeout(long long* timeoutMs);

    /** @brief 设置/读取 SO_LINGER */
    bool setLinger(bool enable, int waitTimeSec = 5);
    bool getLinger(bool& enable, int& waitTimeSec);

    // 网络收发
    int send(const std::string& data) const;
    int send(const void* data, size_t size) const;
    int recv(std::string& data) const;
    int recv(void* data, int length, bool complete = false) const;
    int sendTo(const std::string& data, int flags, InetAddress& sinaddr) const;
    int sendTo(const void* data, size_t size, int flags, InetAddress& sinaddr) const;
    int recvFrom(std::string& data, int flags, InetAddress& sinaddr) const;
    int recvFrom(void* data, int length, int flags, InetAddress& sinaddr) const;

    // 属性访问
    bool isValid() const { return m_sockfd != AFL_INVALID_SOCKET; }

    const AFL_SOCKET& fd() const { return m_sockfd; }

    //template < typename T >
    //Socket& operator<< (const T& t)
    //{
    // T 须为 POD
    //    send(&t, sizeof(t));
    //}

    //template < typename T >
    //Socket& operator>> (const T& t)
    //{
    //    // T 须为 POD
    //    recv(&t, sizeof(t));
    //}
protected:
    const AFL_SOCKET m_sockfd;
};

} // namespace net
} // namespace afl
