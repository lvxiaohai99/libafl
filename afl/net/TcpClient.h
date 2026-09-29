/**
 * @file   TcpClient.h
 * @brief  TCP 客户端
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include "afl/concurrency/Mutex.h"
#include "afl/net/CallBacks.h"
#include "afl/net/InetAddress.h"
#include "afl/base/NonCopy.h"
#include "afl/net/TcpConnection.h"
using afl::concurrency::Mutex;
using afl::time::TimeStamp;

namespace afl
{
namespace net
{
class EventLoop;
class InetAddress;
class TcpConnector;
class ByteBuffer;
class Tcpconnection;

class TcpClient
{
public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr,
              const std::string& clientname = "TcpClient");
    ~TcpClient();

public:
    EventLoop* getLoop() const { return m_loop; }
    AFL_SOCKET fd() const
    {
        assert(m_connection);
        return m_connection->fd();
    }

    void setConnectionCallback(const ConnectionCallback& cb) { m_connectionCallback = cb; }

    void setMessageCallback(const MessageCallback& cb) { m_messageCallback = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_writeCompleteCallback = cb; }

public:
    void connect();
    void reconnect();
    void disconnect();
    void stop();

    bool retry() const { return m_retry; }
    void enableRetry() { m_retry = true; }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

private:
    EventLoop* m_loop;
    TcpConnector* m_connector;
    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;
    bool m_retry;
    bool m_connect;
    TcpConnectionPtr m_connection;
    const std::string m_clientName;
};

} // namespace net
} // namespace afl
