/**
 * @file   TcpServer.cpp
 * @brief  TCP 服务器的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/TcpServer.h"
#include "afl/net/InetAddress.h"
#include "afl/net/TcpAcceptor.h"
#include "afl/net/TcpConnection.h"
#include "afl/net/EventLoop.h"
#include "afl/net/EventLoopThreadPool.h"
#include "afl/log/Log.h"

namespace afl
{
namespace net
{
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
                     const std::string& server_name /* = "TcpServer"*/)
    : m_loop(loop), m_serverAddr(listenAddr.getSockAddrInet()), m_serverName(server_name)
{
    m_acceptor = new TcpAcceptor(loop, listenAddr);
    m_acceptor->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));

    m_connectionCallback = defaultConnectionCallback;
    m_messageCallback = defaultMessageCallback;

    m_evloopThreadPool = new EventLoopThreadPool(m_loop);
}

TcpServer::~TcpServer()
{
    m_loop->assertInLoopThread();

    for (ConnectionMap::iterator it(m_connections.begin()); it != m_connections.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();
    }
}

void TcpServer::setMultiReactorThreads(int numThreads)
{
    m_evloopThreadPool->setMultiReactorThreads(numThreads);
}

void TcpServer::start()
{
    m_evloopThreadPool->start();
    m_loop->runInLoop(std::bind(&TcpAcceptor::listen, m_acceptor));
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    m_loop->assertInLoopThread();
    EventLoop* ioLoop = m_evloopThreadPool->getNextLoop(); // m_loop;

    LOG_INFO("TcpServer::newConnection [%d] from [%s]", sockfd, peerAddr.ipPort().c_str());
    InetAddress localAddr(SocketUtil::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(ioLoop, sockfd, localAddr, peerAddr));
    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    m_connections[sockfd] = conn;
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    m_loop->assertInLoopThread();
    LOG_INFO("TcpServer::removeConnectionInLoop [%d] - %s", conn->fd(),
             conn->peerAddress().ipPort().c_str());
    size_t n = m_connections.erase(conn->fd());
    AFL_UNUSED(n);
    assert(n == 1);

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    LOG_INFO("TcpServer::removeConnectionInLoop [%d] - %s", conn->fd(),
             conn->peerAddress().ipPort().c_str());
}

} // namespace net
} // namespace afl
