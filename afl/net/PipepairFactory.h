/**
 * @file   PipepairFactory.h
 * @brief  pipe、socketpair、eventfd 的封装工厂，可用于线程/进程间通信及同步
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/net/SocketUtil.h"
#include "afl/log/Log.h"
#include "afl/time/TimeStamp.h"
#include "afl/concurrency/Thread.h"
#include <sys/epoll.h>   // for epoll
#include <sys/eventfd.h> // for eventfd

using afl::time::TimeStamp;

namespace afl
{
namespace net
{
/** @brief 成对 fd 工厂 + epoll 限时 wait/notify */
template <typename Factory>
class FdPairFactory
{
public:
    FdPairFactory()
    {
        m_epfd = m_fds[0] = m_fds[1] = -1;
        if (m_factory.create(m_fds) != 0)
        {
            LOG_ERROR("FdPairFactory: create fds failed[%d]", errno);
        }
        SocketUtil::setNonBlocking(m_fds[0]); // m_fds[0] 读端
        SocketUtil::setNonBlocking(m_fds[1]); // m_fds[1] 写端
    }

    ~FdPairFactory()
    {
        close(m_fds[0]);
        close(m_fds[1]);
        close(m_epfd);
    }

    int readFd() { return m_fds[0]; }

    int writeFd() { return m_fds[1]; }

    void closeRead()
    {
        close(m_fds[0]);
        //m_fds[0] = m_fds[1];
    }

    void closeWrite()
    {
        close(m_fds[1]);
        //m_fds[1] = m_fds[0];
    }

    ssize_t write(const void* data, size_t len) { return SocketUtil::write(m_fds[1], data, len); }

    ssize_t read(void* buf, size_t size) { return SocketUtil::read(m_fds[0], buf, size); }

    ssize_t notify()
    {
        char c[1] = {'n'};
        return write(c, 1);
    }

    bool wait(int timeoutMs)
    {
        char c[1];
        if (timeoutMs == 0)
        {
            return read(c, 1) == 1;
        }

        lazyInitEpoll();

        struct epoll_event events[1];
        while (true)
        {
            TimeStamp now(TimeStamp::now());
            int n = epoll_wait(m_epfd, events, 1, timeoutMs);
            if (n < 0)
            {
                LOG_ERROR("FdPairFactory: epoll_wait failed [%d, %d, %d].", m_epfd, timeoutMs,
                          errno);
                return false;
            }
            else if (n == 0)
            {
                return false;
            }

            if (read(c, 1) == 1)
            {
                return true;
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                LOG_ERROR("FdPairFactory: epoll read from[%d] failed[%d]", m_fds[0], errno);
                return false;
            }

            int64_t ms = TimeStamp::timeDiffMS(TimeStamp::now(), now);
            timeoutMs -= ms;
            if (timeoutMs <= 0)
            {
                return false;
            }
        }

        return true;
    }

private:
    void lazyInitEpoll()
    {
        if (m_epfd > 0)
            return;

        m_epfd = epoll_create(2);
        if (m_epfd == -1)
        {
            LOG_ERROR("FdPairFactory: epoll_create failed[%d]", errno);
        }
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN;
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_fds[0], &event) != 0)
        {
            LOG_ERROR("FdPairFactory: epoll_ctl failed. [%d, %d, %s]", m_epfd, m_fds[0], errno);
        }
    }

    void close(int fd)
    {
        if (fd != -1)
        {
            ::close(fd);
            fd = -1;
        }
    }

private:
    Factory m_factory;
    int m_fds[2];
    int m_epfd;
};

class PipePairGenerator
{
public:
    int create(int fds[2]) { return ::pipe(fds); }
};

class SocketPairGenerator
{
public:
    int create(int fds[2])
    {
        //return socketpair2(AF_UNIX, SOCK_STREAM, 0, fds);
        return ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    }

private:
    int socketpair2(int af, int type, int protocol, int fd[2]) // socketpair 的替代实现
    {
        int listen_socket;
        struct sockaddr_in sin[2];
        int len;

        if (type != SOCK_STREAM) // 以下逻辑仅适用于 SOCK_STREAM
            return -1;
        /* 创建临时监听 socket，端口可任意 */
        listen_socket = socket(af, type, protocol);
        if (listen_socket < 0)
        {
            perror("creating listen_socket");
            return -1;
        }
        sin[0].sin_family = af;
        sin[0].sin_port = 0; /* 由系统分配端口 */
        sin[0].sin_addr.s_addr = INADDR_ANY;
        if (bind(listen_socket, (struct sockaddr*)&sin[0], sizeof(sin[0])) < 0)
        {
            perror("bind");
            return -1;
        }
        len = sizeof(sin[0]);
        /* 读取实际绑定端口，供客户端 connect */
        if (getsockname(listen_socket, (struct sockaddr*)&sin[0], (socklen_t*)&len) < 0)
        {
            perror("getsockname");
            return -1;
        }
        /* 进入 listen 状态 */
        if (listen(listen_socket, 5) < 0)
        {
            perror("listen");
            return -1;
        }
        /* 创建客户端 socket */
        fd[1] = socket(af, type, protocol);
        if (fd[1] < 0)
        {
            perror("creating client_socket");
            return -1;
        }
        /* 客户端 socket 非阻塞 connect */
        fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL, 0) | O_NDELAY);
        if (connect(fd[1], (struct sockaddr*)&sin[0], sizeof(sin[0])) < 0)
        {
            perror("connect");
            return -1;
        }
        /* 监听端 accept 自连建立的连接 */
        len = sizeof(sin[1]);
        if ((fd[0] = accept(listen_socket, (struct sockaddr*)&sin[1], (socklen_t*)&len)) < 0)
        {
            perror("accept");
            return -1;
        }
        /* 客户端 socket 恢复阻塞模式 */
        fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL, 0) & ~O_NDELAY);
        close(listen_socket);
        return 0;
    }
};

class TcpPairGenerator
{
public:
    int create(int fds[2])
    {
        int listenSock = SocketUtil::createSocketAndListen("0.0.0.0", 0, 5);
        SocketUtil::setNonBlocking(listenSock);

        int clientSock = SocketUtil::createSocket();
        struct sockaddr_in addr = SocketUtil::getLocalAddr(listenSock);
        SocketUtil::setNonBlocking(clientSock);

        int ret = SocketUtil::connect(clientSock, addr); // 非阻塞 connect
        if (ret != 0 && errno != SOCK_ERR_EINPROGRESS)   // 进行中 errno
        {
            LOG_ERROR("TcpPairGenerator: connect failed[%d][%d][%d].\n", clientSock, ret, errno);
            SocketUtil::closeSocket(listenSock);
            SocketUtil::closeSocket(clientSock);
            return -1;
        }

        struct sockaddr_in addr2;
        int srvSock = SocketUtil::accept(listenSock, &addr2);
        if (srvSock < 0 || errno != EINPROGRESS)
        {
            LOG_ERROR("TcpPairGenerator: accept failed[%d][%d].\n", srvSock, errno);
            SocketUtil::closeSocket(listenSock);
            SocketUtil::closeSocket(clientSock);
            return -1;
        }

        SocketUtil::setNonBlocking(srvSock, false);
        SocketUtil::setNonBlocking(clientSock, false);
        fds[0] = srvSock;
        fds[1] = clientSock;

        SocketUtil::closeSocket(listenSock);

        return 0;
    }
};

class EventFdGenerator
{
public:
    int create(int fds[2])
    {
        fds[0] = fds[1] = createEventfd();
        return 0;
    }

private:
    int createEventfd()
    {
        int efd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (efd < 0)
        {
            LOG_ERROR("create eventfd failed when EventLoop::EventLoop");
            assert(efd);
        }
        LOG_INFO("EventFdGenerator::createEventfd [%d]", efd);
        return efd;
    }
};

typedef FdPairFactory<PipePairGenerator> PipePairFactory;
typedef FdPairFactory<SocketPairGenerator> SocketPairFactory;
typedef FdPairFactory<TcpPairGenerator> TcpPairFactory;
typedef FdPairFactory<EventFdGenerator> EventFdPairFactory;

} // namespace net
} // namespace afl
