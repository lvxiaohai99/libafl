/**
 * @file   TcpClient.cpp
 * @brief  TCP 客户端的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/TcpClient.h"
#include "afl/net/EventLoop.h"
#include "afl/net/InetAddress.h"
#include "afl/net/TcpConnection.h"
#include "afl/net/TcpConnector.h"
#include "afl/net/SocketUtil.h"
#include "afl/log/Log.h"

using namespace afl::base;
namespace afl
{
namespace net
{
namespace detail
{
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(TcpConnectorPtr connector)
{
    SAFE_DELETE(connector);
}
} // namespace detail

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& clientname)
    : m_loop(loop), m_connectionCallback(defaultConnectionCallback),
      m_messageCallback(defaultMessageCallback), m_retry(false), m_connect(true),
      m_clientName(clientname)
{
    m_connector = new TcpConnector(loop, serverAddr);
    m_connector->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
}

TcpClient::~TcpClient()
{
    detail::removeConnection(m_loop, m_connection);
    m_connector->stop();
    m_loop->runInLoop(std::bind(&detail::removeConnector, m_connector));
}

void TcpClient::connect()
{
    m_connect = true;
    m_connector->connect();
}

void TcpClient::reconnect()
{
    if (m_connection)
        removeConnection(m_connection);

    m_connect = true;
    m_connector->reconnect();
}

void TcpClient::disconnect()
{
    m_connect = false;

    if (m_connection)
    {
        m_connection->shutdown();
    }
}

void TcpClient::stop()
{
    m_connect = false;
    m_connector->stop();
}

void TcpClient::newConnection(int sockfd)
{
    LOG_INFO("TcpClient::newConnection [%d]", sockfd);
    m_loop->assertInLoopThread();
    InetAddress peerAddr(SocketUtil::getPeerAddr(sockfd));
    InetAddress localAddr(SocketUtil::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(m_loop, sockfd, localAddr, peerAddr));

    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
    conn->connectEstablished();

    m_connection = conn;
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    m_loop->assertInLoopThread();
    assert(m_loop == conn->getLoop());
    assert(m_connection == conn);

    m_loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));

    if (m_retry && m_connect)
    {
        m_connector->reconnect();
    }
}

} // namespace net
} // namespace afl
