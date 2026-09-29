/**
 * @file   TcpConnector.cpp
 * @brief  客户端连接器的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/TcpConnector.h"
#include "afl/net/EventLoop.h"
#include "afl/net/Channel.h"
#include "afl/log/Log.h"

namespace afl
{
namespace net
{
const int TcpConnector::kMaxRetryDelayMs = 5 * 1000;
const int TcpConnector::kInitRetryDelayMs = 500;

TcpConnector::TcpConnector(EventLoop* loop, const InetAddress& serverAddr)
    : m_state(kDisconnected), m_connect(false), m_loop(loop), m_serverAddr(serverAddr),
      m_connChannel(NULL), retryDelayMs(kInitRetryDelayMs), retryTimer(-1)
{
}

TcpConnector::~TcpConnector() {}

void TcpConnector::cancelPendingRetry()
{
    if (retryTimer > 0)
    {
        m_loop->cancelTimer(retryTimer);
        retryTimer = -1;
    }
}

void TcpConnector::connect()
{
    cancelPendingRetry();
    m_connect = true;
    m_loop->runInLoop(std::bind(&TcpConnector::connectInLoop, this));
}


void TcpConnector::reconnect()
{
    m_loop->assertInLoopThread();

    if (m_state == kConnecting)
        return;

    setState(kDisconnected);
    retryDelayMs = kInitRetryDelayMs;

    cancelPendingRetry();
    m_connect = true;
    connectInLoop();
}

void TcpConnector::connectInLoop()
{
    m_loop->assertInLoopThread();
    //    assert(m_state == kDisconnected);
    if (m_state == kDisconnected && m_connect)
    {
        connectServer();
    }
}

void TcpConnector::connectServer()
{
    AFL_SOCKET sockfd = SocketUtil::createSocket(); //::createNonblockingOrDie();
    SocketUtil::setNonBlocking(sockfd);

    int ret = SocketUtil::connect(sockfd, m_serverAddr.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connectEstablished(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_ERROR("TcpConnector::connectServer() error[%d]", savedErrno);
        SocketUtil::closeSocket(sockfd);
        break;

    default:
        LOG_ERROR("TcpConnector::connectServer() unexpected error[%d]", savedErrno);
        SocketUtil::closeSocket(sockfd);
        // m_connectErrorCallback();
        break;
    }
}

void TcpConnector::connectEstablished(AFL_SOCKET sock)
{
    LOG_INFO("TcpConnector::connectEstablished : [%d]", sock);
    setState(kConnecting);
    if (m_connChannel)
        delete m_connChannel;

    m_connChannel = new Channel(m_loop, sock);
    m_connChannel->setWriteCallback(std::bind(&TcpConnector::handleWrite, this));
    m_connChannel->setErrorCallback(std::bind(&TcpConnector::handleError, this));

    m_connChannel->enableWriting();
    LOG_INFO("TcpConnector::connectEstablished : [%d]", sock);
}

void TcpConnector::stop()
{
    cancelPendingRetry();
    m_connect = false;
    m_loop->queueInLoop(std::bind(&TcpConnector::stopInLoop, this));
}

void TcpConnector::stopInLoop()
{
    m_loop->assertInLoopThread();
    if (m_state == kConnecting)
    {
        setState(kDisconnected);
        AFL_SOCKET sockfd = disableChannel();
        retry(sockfd);
    }
}

AFL_SOCKET TcpConnector::disableChannel()
{
    m_connChannel->disableAll(); // 从poller中移除，不再关注任何事件
    m_connChannel->remove();
    AFL_SOCKET sockfd = m_connChannel->fd();
    return sockfd;
}

//连接远端socket成功
void TcpConnector::handleWrite()
{
    LOG_INFO("TcpConnector::handleWrite : [%d]", m_connChannel->fd());
    if (m_state ==
        kConnecting) //连接建立时注册Channel可写事件，此时响应可写，将socket返回，并禁用Channel
    {
        AFL_SOCKET sockfd = disableChannel();
        int err = SocketUtil::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN("TcpConnector::handleWrite - SO_ERROR = [%d][%d][%s]", sockfd, err,
                     strerror(err));
            retry(sockfd);
        }
        else if (SocketUtil::isSelfConnect(sockfd))
        {
            LOG_WARN("TcpConnector::handleWrite - Self connect = [%d]", sockfd);
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (m_connect)
            {
                m_newConnCallBack(sockfd);
            }
            else
            {
                SocketUtil::closeSocket(sockfd);
            }
        }
    }
    else
    {
        assert(m_state == kDisconnected);
    }
}

void TcpConnector::handleError()
{
    LOG_ERROR("TcpConnector::handleError(): fd = [%d], state = [%d]", m_connChannel->fd(), m_state);
    if (m_state == kConnecting)
    {
        AFL_SOCKET sockfd = disableChannel();
        int err = SocketUtil::getSocketError(sockfd);
        LOG_ERROR("TcpConnector::handleError() SO_ERROR = [%d][%s]", err, strerror(err));
        err++; /* discard warning */
        retry(sockfd);
    }
}

void TcpConnector::retry(AFL_SOCKET sockfd)
{
    SocketUtil::closeSocket(sockfd);
    setState(kDisconnected);
    if (m_connect)
    {
        LOG_INFO("TcpConnector::retry; Retry connecting to [%s]", m_serverAddr.ipPort().c_str());
        retryTimer =
            m_loop->addTimer(std::bind(&TcpConnector::connectInLoop, this), retryDelayMs / 1000.0);
        retryDelayMs = std::min(retryDelayMs * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_INFO("TcpConnector::retry: do not connect");
    }
}

} // namespace net
} // namespace afl
