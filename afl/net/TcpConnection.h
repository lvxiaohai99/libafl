/**
 * @file   TcpConnection.h
 * @brief  TCP 连接封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include "afl/base/NonCopy.h"
#include "afl/base/Any.h"
#include "afl/net/CallBacks.h"
#include "afl/net/InetAddress.h"
#include "afl/net/Socket.h"
#include "afl/net/ByteBuffer.h"
#include "afl/net/Channel.h"
#include <memory> //for enable_shared_from_this

namespace afl
{
namespace net
{
class Channel;
class EventLoop;
class Socket;
class InetAddress;
using afl::time::TimeStamp;

class TcpConnection : afl::base::NonCopy, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int sockfd, const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

public:
    EventLoop* getLoop() const { return m_loop; }
    AFL_SOCKET fd() const { return m_socket->fd(); }
    const InetAddress& localAddress() const { return m_localAddr; }
    const InetAddress& peerAddress() const { return m_peerAddr; }
    bool connected() const { return m_state == kConnected; }

    void setConnectionCallback(const ConnectionCallback& cb) { m_connectionCallback = cb; }
    void setMessageCallback(const MessageCallback& cb) { m_messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_writeCompleteCallback = cb; }
    void setCloseCallback(const CloseCallback& cb) { m_closeCallback = cb; }

    void enableReading() { m_channel->enableReading(); }
    void disableReading() { m_channel->disableReading(); }
    void enableWriting() { m_channel->enableWriting(); }
    void disableWriting() { m_channel->disableWriting(); }
    void disableAll() { m_channel->disableAll(); }

    void setNoDelay(bool on) { m_socket->setNoDelay(on); }

    void setContext(const afl::base::any& context) { m_context = context; }
    const afl::base::any getContext() const { return m_context; }
    afl::base::any* getMutableContext() { return &m_context; }

    void connectEstablished(); // called when TcpServer accepts a new connection
    void connectDestroyed();   // called when TcpServer has removed me from its map

    void send(const void* data, size_t len);
    void send(const std::string& buffer);
    void send(ByteBuffer* buffer);

    void shutdown();

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    const char* getState(StateE);
    void handleRead(TimeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const void* data, size_t len);
    void sendInLoop(const std::string& buffer);
    void shutdownInLoop();
    void setState(StateE s) { m_state = s; }

private:
    EventLoop* m_loop;
    StateE m_state;
    Socket* m_socket;
    Channel* m_channel;
    const InetAddress m_localAddr;
    const InetAddress m_peerAddr;

    afl::base::any m_context;

    ByteBuffer m_inputBuffer;
    ByteBuffer m_outputBuffer; // FIXME: 输出队列宜改为 list<Buffer> 以降低大块拷贝

    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;
    CloseCallback m_closeCallback;
};

} // namespace net
} // namespace afl
