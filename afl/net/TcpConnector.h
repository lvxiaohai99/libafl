/**
 * @file   TcpConnector.h
 * @brief  客户端连接器，连接远程 socket
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/net/SocketUtil.h"
#include "afl/net/InetAddress.h"
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"

namespace afl
{
namespace net
{
class Socket;
class Channel;
class EventLoop;
class InetAddress;

class TcpConnector : afl::base::NonCopy
{
public:
    typedef std::function<void(AFL_SOCKET)> NewConnectionCallback;

public:
    TcpConnector(EventLoop* loop, const InetAddress& serverAddr);
    ~TcpConnector();

    void setNewConnectionCallback(const NewConnectionCallback& callback)
    {
        m_newConnCallBack = callback;
    }

    const InetAddress& serverAddress() const { return m_serverAddr; }

    void connect();
    void reconnect();
    void stop();

private:
    void connectInLoop();
    void connectServer();
    void connectEstablished(AFL_SOCKET sock);
    void stopInLoop();

    void handleWrite();
    void handleError();
    AFL_SOCKET disableChannel();
    void retry(AFL_SOCKET sockfd);
    void cancelPendingRetry();

    enum States
    {
        kDisconnected,
        kConnecting,
        kConnected
    };
    void setState(States s) { m_state = s; }

private:
    States m_state;
    bool m_connect;
    EventLoop* m_loop;
    const InetAddress m_serverAddr;
    Channel* m_connChannel;
    NewConnectionCallback m_newConnCallBack;

    int retryDelayMs;
    int retryTimer;

    static const int kMaxRetryDelayMs;
    static const int kInitRetryDelayMs;
};

typedef TcpConnector* TcpConnectorPtr;

} // namespace net
} // namespace afl
