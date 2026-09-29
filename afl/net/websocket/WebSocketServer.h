/**
 * @file   WebSocketServer.h
 * @brief  WebSocket 服务器
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/TcpServer.h"
#include "afl/net/CallBacks.h"
#include "afl/net/http/HttpProtocol.h"
#include "afl/net/websocket/WebSocket.h"
#include <string>
namespace afl
{
namespace net
{
class EventLoop;
class HttpRequest;
class HttpResponse;
class InetAddress;

namespace ws
{
class WsServer : public afl::net::TcpServer
{
public:
    typedef std::function<void(const TcpConnectionPtr&)> OnOpenCallback;
    typedef std::function<void(const TcpConnectionPtr&)> OnCloseCallback;
    typedef std::function<void(const TcpConnectionPtr&, const std::vector<char>&, TimeStamp)>
        OnMessageCallback;

public:
    WsServer(EventLoop* loop, const InetAddress& listenAddr,
             const std::string& servername = "WsServer");
    ~WsServer();

public:
    void setOnOpen(const OnOpenCallback& cb) { m_onopen = cb; }

    void setOnClose(const OnCloseCallback& cb) { m_onclose = cb; }

    void setOnMessage(const OnMessageCallback& cb) { m_onmessage = cb; }

    void sendText(const TcpConnectionPtr& conn, const char* data, size_t size);
    void sendBinary(const TcpConnectionPtr& conn, const char* data, size_t size);
    void send(const TcpConnectionPtr& conn, const char* data, size_t size,
              WsFrameType type = WS_TEXT_FRAME);
    void close(const TcpConnectionPtr& conn, WsCloseReason code = WS_CLOSE_NORMAL,
               const char* reason = NULL);

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime);
    void handshake(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime);

private:
    WsServer(const WsServer&);
    WsServer& operator=(const WsServer&);

    OnOpenCallback m_onopen;
    OnCloseCallback m_onclose;
    OnMessageCallback m_onmessage;
};

} // namespace ws
} // namespace net
} // namespace afl
