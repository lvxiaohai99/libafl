/**
 * @file   TcpConnection.cpp
 * @brief  TCP 连接的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/TcpConnection.h"
#include "afl/net/Socket.h"
#include "afl/net/EventLoop.h"
#include "afl/net/Channel.h"
#include "afl/log/Log.h"
#include "afl/base/SmartAssert.h"
namespace afl
{
namespace net
{
void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO("defaultConnectionCallback : [%s]<->[%s] [%s]\n",
             conn->localAddress().ipPort().c_str(), conn->peerAddress().ipPort().c_str(),
             conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime)
{
    LOG_INFO("defaultMessageCallback : [%d][%s]", conn->fd(), buf->toString().c_str());
}

TcpConnection::TcpConnection(EventLoop* loop, int sockfd, const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : m_loop(loop), m_state(kConnecting), m_localAddr(localAddr.getSockAddrInet()),
      m_peerAddr(peerAddr.getSockAddrInet())
{
    m_socket = new Socket(sockfd);
    m_socket->setKeepAlive(true);
    m_socket->setNoDelay(true);
    m_socket->setNonBlocking();

    m_channel = new Channel(loop, sockfd);
    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::TcpConnection(), [%0x] [%d][%0x][%0x]", this, m_socket->fd(), m_socket,
             m_channel);
}

TcpConnection::~TcpConnection()
{
    //LOG_INFO("TcpConnection::~TcpConnection(),[%0x] [%d][%0x][%0x]", this, m_socket->fd(), m_socket, m_channel);
    // AFL_ASSERT(m_state == kDisconnected)(m_state); // 异常断开或 EventLoop 退出时，连接未必已是 disconnected
    SAFE_DELETE(m_socket);
    SAFE_DELETE(m_channel);
}

const char* TcpConnection::getState(StateE state)
{
    switch (state)
    {
    case kDisconnected:
        return "TcpDisconnected";
        break;
    case kConnecting:
        return "TcpConnecting";
        break;
    case kConnected:
        return "TcpConnected";
        break;
    case kDisconnecting:
        return "TcpDisconnecting";
        break;
    default:
        assert(0);
        break;
    }
    return "null";
}

void TcpConnection::send(const void* data, size_t len)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            // 拷贝数据并持有连接引用，避免异步执行时调用方缓冲区已失效
            std::string buf(static_cast<const char*>(data), len);
            m_loop->runInLoop(std::bind(static_cast<void (TcpConnection::*)(const std::string&)>(
                                            &TcpConnection::sendInLoop),
                                        shared_from_this(), buf));
        }
    }
}

void TcpConnection::send(const std::string& buffer)
{
    send(buffer.data(), buffer.size());
}

void TcpConnection::send(ByteBuffer* buffer)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(buffer->peek(), buffer->readableBytes());
            buffer->retrieveAll();
        }
        else
        {
            m_loop->runInLoop(std::bind(static_cast<void (TcpConnection::*)(const std::string&)>(
                                            &TcpConnection::sendInLoop),
                                        shared_from_this(), buffer->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const std::string& buffer)
{
    sendInLoop(buffer.data(), buffer.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    m_loop->assertInLoopThread();
    if (m_state == kDisconnected)
    {
        LOG_WARN("TcpConnection::sendInLoop [%d]disconnected, give up writing", m_socket->fd());
        return;
    }

    int nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    // 如果当前连接尚没有注册可写事件（比如直接调用send接口），并且发缓冲区为空
    // 就直接发送数据，发成功则回调写完成事件；
    if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = m_socket->send((const char*)data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && m_writeCompleteCallback)
            {
                m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop error, fd[%d], error[%d]", m_socket->fd(),
                          errno);
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }
    AFL_ASSERT(remaining <= len)(remaining)(len)(m_socket->fd());

    if (len > 65535)
    {
        return;
    }

    if ((m_outputBuffer.readableBytes() + len) > 65535)
    {
        return;
    }
    //如果发送成功且数据尚未发送完毕，则将剩余数据保存到输出缓冲区
    if (!faultError && remaining > 0)
    {
        m_outputBuffer.write(static_cast<const char*>(data) + nwrote, remaining);
        if (!m_channel->isWriting())
        {
            m_channel->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (m_state == kConnected)
    {
        setState(kDisconnecting);
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{
    m_loop->assertInLoopThread();
    if (!m_channel->isWriting()) // 如果不再关注可写事件，说明数据发送完
    {
        SocketUtil::shutdownWrite(m_socket->fd()); // 仅仅关闭写端，因为可能读端还有数据要
    }
}

void TcpConnection::connectEstablished()
{
    LOG_INFO("TcpConnection::connectEstablished fd = %d, state = %s", m_socket->fd(),
             getState(m_state));
    m_loop->assertInLoopThread();
    AFL_ASSERT(m_state == kConnecting)(m_state);
    setState(kConnected);
    m_channel->enableReading();

    TcpConnectionPtr sp_this(shared_from_this());
    m_connectionCallback(sp_this);
}

void TcpConnection::connectDestroyed()
{
    LOG_INFO("TcpConnection::connectDestroyed fd = %d, state = %s", m_socket->fd(),
             getState(m_state));
    m_loop->assertInLoopThread();
    if (m_state == kConnected)
    {
        setState(kDisconnected);
        TcpConnectionPtr sp_this(shared_from_this());
        m_connectionCallback(sp_this);
    }
    m_channel->disableAll();
    m_channel->remove();
}

void TcpConnection::handleRead(TimeStamp receiveTime)
{
    LOG_DEBUG("TcpConnection::handleRead fd = %d, state = %s", m_socket->fd(), getState(m_state));
    m_loop->assertInLoopThread();
    std::string data;
    size_t n = m_socket->recv(data);
    m_inputBuffer.write(data);
    if (n > 0)
    {
        m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    LOG_DEBUG("TcpConnection::handleWrite fd = %d, state = %s", m_socket->fd(), getState(m_state));
    m_loop->assertInLoopThread();

    if (m_channel->isWriting())
    {
        size_t n = m_socket->send(m_outputBuffer.peek(), m_outputBuffer.readableBytes());
        if (n > 0)
        {
            m_outputBuffer.retrieve(n);
            LOG_INFO("TcpConnection::handleWrite fd = %d, send = %d, reserve = %d", m_socket->fd(),
                     n, m_outputBuffer.readableBytes());
            if (m_outputBuffer.readableBytes() ==
                0) // 缓冲区为空，数据发完毕，删除该channel上的可写事件
            {
                m_channel->disableWriting();
                if (m_writeCompleteCallback)
                {
                    m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
                }
                if (m_state == kDisconnecting) // 数据发完毕，且连接已断开
                {
                    shutdownInLoop(); // 关闭socket的可
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite, send fail fd = %d, state = %s, send = %d",
                      m_socket->fd(), getState(m_state), n);
            if (m_state == kDisconnecting)
            {
                shutdownInLoop();
            }
        }
    }
    else
    {
        LOG_ERROR("TcpConnection::handleWrite,  no more writing, fd = %d, state = %s",
                  m_socket->fd(), getState(m_state));
    }
}

void TcpConnection::handleClose()
{
    m_loop->assertInLoopThread();
    AFL_ASSERT(m_state == kConnected || m_state == kDisconnecting)(m_state)(m_socket->fd());
    setState(kDisconnected);
    m_channel->disableAll();

    m_connectionCallback(shared_from_this());

    m_closeCallback(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = SocketUtil::getSocketError(m_channel->fd());
    LOG_ERROR("TcpConnection::handleError [%d], SO_ERROR = %d", m_channel->fd(), err);
    err++;
}

} // namespace net
} // namespace afl
