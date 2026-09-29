/**
 * @file   Socket.cpp
 * @brief  socket RAII 封装的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/Socket.h"
#include "afl/base/Exception.h"
#include "afl/net/InetAddress.h"
namespace afl
{
namespace net
{
const static int MAX_RECV_SIZE = 64 * 1024; // 璁剧疆64k ?

/*********
recv read write send 函数的返回说:
    < 0 ：出;
    = 0 ：连接关;
    > 0 : 接收/发数据大
除此之外，非阻塞模式下返回 < 0 并且 (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)的情况下认为连接是正常的，继续接/发
*********/

Socket::Socket(AFL_SOCKET fd) : m_sockfd(fd) {}

Socket::~Socket()
{
    SocketUtil::closeSocket(m_sockfd);
}

bool Socket::bind(const char* ip, int port)
{
    if (!isValid())
    {
        return false;
    }

    return SocketUtil::bind(m_sockfd, ip, port) == 0;
}

bool Socket::bind(const InetAddress& addr)
{
    if (!isValid())
    {
        return false;
    }
    return SocketUtil::bind(m_sockfd, addr.getSockAddrInet()) == 0;
}

bool Socket::listen(int backlog /*= 5*/) const
{
    if (!isValid())
    {
        return false;
    }

    int ret = AFL_LISTEN(m_sockfd, backlog);
    if (ret == -1)
    {
        return false;
    }
    return true;
}

AFL_SOCKET Socket::accept(AFL_SOCKADDR_IN* peerAddr) const
{
    return SocketUtil::accept(m_sockfd, peerAddr);
}

AFL_SOCKET Socket::accept(InetAddress* peerAddr) const
{
    AFL_SOCKADDR_IN addr;
    ::memset(&addr, 0, sizeof(addr));
    AFL_SOCKET connfd = SocketUtil::accept(m_sockfd, &addr);
    if (connfd > 0)
    {
        peerAddr->setSockAddrInet(addr);
    }
    return connfd;
}

bool Socket::connect(const char* ip, int port)
{
    if (!isValid())
    {
        return false;
    }

    return SocketUtil::connect(m_sockfd, ip, port) == 0;
}

void Socket::close()
{
    if (isValid())
    {
        AFL_CLOSE(m_sockfd);
    }
}

int Socket::send(const std::string& data) const
{
    return send(data.c_str(), data.size());
}

int Socket::send(const void* data, size_t size) const
{
    int len = AFL_SEND(m_sockfd, data, size, MSG_NOSIGNAL);
    //if (len == -1)     // error
    //else if (len == 0) // connection is closed
    //else               // ok
    return len;
}

int Socket::recv(std::string& data) const
{
    char buf[MAX_RECV_SIZE + 1] = {0};

    while (true)
    {
        int len = recv(buf, MAX_RECV_SIZE, false);
        if (len == 0) // 连接已关
        {
            return 0;
        }
        else if (len < 0)
        {
            if (SOCK_ERR_RW_RETRY(AFL_SOCKET_ERROR))
                continue;
            else if (AFL_SOCKET_ERROR == SOCK_ERR_EWOULDBLOCK)
                break;
            else
                return len; // 发生了错
            break;
        }
        else
        {
            data.insert(data.end(), buf, buf + len);
            if (len < MAX_RECV_SIZE) //全部接收完成
                break;
        }
    }
    return data.size();
}

int Socket::recv(void* data, int length, bool complete /*= false */) const
{
    int received = 0;
    if (complete)
    {
        while (received != length)
        {
            int len = AFL_RECV(m_sockfd, (char*)data + received, length - received, 0);
            if (len == -1)
            {
                printf("status == -1, errno == [%d], in Socket::recv\n", errno);
                return len;
            }
            else if (len == 0)
            {
                return len;
            }
            else
            {
                received += len;
            }
        }
    }
    else
    {
        received = AFL_RECV(m_sockfd, (char*)data, length, 0);
    }
    return received;
}

int Socket::sendTo(const std::string& data, int flags, InetAddress& sinaddr) const
{
    return sendTo(data.c_str(), data.size(), flags, sinaddr);
}

int Socket::sendTo(const void* data, size_t size, int flags, InetAddress& sinaddr) const
{
    int len = AFL_SENDTO(m_sockfd, data, size, flags, sinaddr, sinaddr.addressLength());

    return len;
}

int Socket::recvFrom(std::string& data, int flags, InetAddress& sinaddr) const
{
    char buf[MAX_RECV_SIZE + 1] = {0};

    while (true)
    {
        int len = recvFrom(buf, MAX_RECV_SIZE, flags, sinaddr);
        if (len == -1 || len == 0)
            break;
        data.insert(data.end(), buf, buf + len);
        if (len < MAX_RECV_SIZE)
            break;
    }
    return data.size();
}

int Socket::recvFrom(void* data, int length, int flags, InetAddress& sinaddr) const
{
    socklen_t slen;
    int len = AFL_RECVFROM(m_sockfd, data, length, flags, sinaddr, &slen);
    if (slen != sinaddr.addressLength())
        throw afl::base::Exception("unknown protocol type(in Socket::RecvFrom)");
    return len;
}

bool Socket::setNonBlocking(bool on /*= true*/)
{
    SocketUtil::setNonBlocking(m_sockfd, on);
    return true;
}

bool Socket::setNoDelay(bool on /*= true*/)
{
    return SocketUtil::setNoDelay(m_sockfd, on) == 0;
}

bool Socket::setReuseAddr(bool on /*= true*/)
{
    return SocketUtil::setReuseAddr(m_sockfd, on) == 0;
}

bool Socket::setReusePort(bool on /*= true*/)
{
    return SocketUtil::setReusePort(m_sockfd, on) == 0;
}

bool Socket::setKeepAlive(bool on /*= true*/)
{
    return SocketUtil::setKeepAlive(m_sockfd, on) == 0;
}

bool Socket::setSendBuffer(int size)
{
    return SocketUtil::setSendBuffer(m_sockfd, size) == 0;
}

bool Socket::getSendBuffer(int* size)
{
    return SocketUtil::getSendBuffer(m_sockfd, size) == 0;
}

bool Socket::setRecvBuffer(int size)
{
    return SocketUtil::setRecvBuffer(m_sockfd, size) == 0;
}

bool Socket::getRecvBuffer(int* size)
{
    return SocketUtil::getRecvBuffer(m_sockfd, size) == 0;
}

bool Socket::setSendTimeout(long long timeoutMs)
{
    return SocketUtil::setSendTimeout(m_sockfd, timeoutMs) == 0;
}

bool Socket::getSendTimeout(long long* timeoutMs)
{
    return SocketUtil::getSendTimeout(m_sockfd, timeoutMs) == 0;
}

bool Socket::setRecvTimeout(long long timeoutMs)
{
    return SocketUtil::setRecvTimeout(m_sockfd, timeoutMs) == 0;
}

bool Socket::getRecvTimeout(long long* timeoutMs)
{
    return SocketUtil::getRecvTimeout(m_sockfd, timeoutMs) == 0;
}

bool Socket::setLinger(bool enable, int waitTimeSec /*= 5*/)
{
    linger lngr;
    lngr.l_onoff = enable ? 1 : 0;
    lngr.l_linger = waitTimeSec;

    return AFL_SETSOCKOPT(m_sockfd, SOL_SOCKET, SO_LINGER, &lngr, sizeof(linger)) == 0;
}

bool Socket::getLinger(bool& enable, int& waitTimeSec)
{
    linger lngr;
    if (AFL_GETSOCKOPT(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &lngr, sizeof(lngr)) == 0)
    {
        enable = lngr.l_onoff;
        waitTimeSec = lngr.l_linger;
        return true;
    }
    return false;
}

} // namespace net
} // namespace afl
