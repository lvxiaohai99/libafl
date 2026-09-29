/**
 * @file   WebSocketClient.h
 * @brief  WebSocket 客户端
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/CallBacks.h"
#include "afl/net/InetAddress.h"
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
class TcpClient;

namespace ws
{
class WsClient
{
public:
    typedef std::function<void(const TcpConnectionPtr&)> OnOpenCallback;
    typedef std::function<void(const TcpConnectionPtr&)> OnCloseCallback;
    typedef std::function<void(const TcpConnectionPtr&, const std::vector<char>&, TimeStamp)>
        OnMessageCallback;

public:
    WsClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& url,
             const std::string& cilentname = "WsClient");
    ~WsClient();

public:
    void setOnOpen(const OnOpenCallback& cb) { m_onopen = cb; }

    void setOnClose(const OnCloseCallback& cb) { m_onclose = cb; }

    void setOnMessage(const OnMessageCallback& cb) { m_onmessage = cb; }

    void connect();
    void sendPing()
    {
        std::string empty = "hello";
        sendData(m_conn, WS_OPCODE_PING, empty.size(), empty.begin(), empty.end());
    }
    void sendText(const TcpConnectionPtr& conn, const char* data, size_t size)
    {
        sendData(conn, WS_OPCODE_TEXT, size, data, data + size);
    }
    void sendBinary(const TcpConnectionPtr& conn, const char* data, size_t size)
    {
        sendData(conn, WS_OPCODE_BINARY, size, data, data + size);
    }
    void close(const TcpConnectionPtr& conn, WsCloseReason code = WS_CLOSE_NORMAL,
               const char* reason = NULL);

    void setCustomOption(const std::string op);
    void resetCustomOption() { m_customOptions.clear(); }

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime);
    void sendHandshake(const TcpConnectionPtr& conn);
    int parseHandshakeResponse(const TcpConnectionPtr& conn, ByteBuffer* buf);

    template <class Iterator>
    void sendData(const TcpConnectionPtr& conn, WsOpcode opcode, uint64_t message_size,
                  Iterator message_begin, Iterator message_end);
    void send(const TcpConnectionPtr& conn, const std::vector<uint8_t>& data);

private:
    WsClient(const WsClient&);
    WsClient& operator=(const WsClient&);

    OnOpenCallback m_onopen;
    OnCloseCallback m_onclose;
    OnMessageCallback m_onmessage;

    afl::net::TcpClient* m_client;
    std::string m_url;
    bool m_useMask; /// true
    TcpConnectionPtr m_conn;

    std::vector<std::string> m_customOptions;
    const afl::net::InetAddress m_serverAddr;
};


template <class Iterator>
void WsClient::sendData(const TcpConnectionPtr& conn, WsOpcode opcode, uint64_t message_size,
                        Iterator message_begin, Iterator message_end)
{
    // TODO：掩码键须来自高质量随机数生成器（RFC 6455）
    if (!conn)
        return;

    // 固定掩码键（生产环境应使用密码学安全随机数，见上方 TODO）
    const static uint8_t masking_key[4] = {0x12, 0x34, 0x56, 0x78};

    std::vector<uint8_t> header;
    header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) +
                      (m_useMask ? 4 : 0),
                  0);
    header[0] = 0x80 | opcode;
    if (false) {}
    else if (message_size < 126)
    {
        header[1] = (message_size & 0xff) | (m_useMask ? 0x80 : 0);
        if (m_useMask)
        {
            header[2] = masking_key[0];
            header[3] = masking_key[1];
            header[4] = masking_key[2];
            header[5] = masking_key[3];
        }
    }
    else if (message_size < 65536)
    {
        header[1] = 126 | (m_useMask ? 0x80 : 0);
        header[2] = (message_size >> 8) & 0xff;
        header[3] = (message_size >> 0) & 0xff;
        if (m_useMask)
        {
            header[4] = masking_key[0];
            header[5] = masking_key[1];
            header[6] = masking_key[2];
            header[7] = masking_key[3];
        }
    }
    else // TODO：超大帧分支需补充覆盖率测试
    {
        header[1] = 127 | (m_useMask ? 0x80 : 0);
        header[2] = (message_size >> 56) & 0xff;
        header[3] = (message_size >> 48) & 0xff;
        header[4] = (message_size >> 40) & 0xff;
        header[5] = (message_size >> 32) & 0xff;
        header[6] = (message_size >> 24) & 0xff;
        header[7] = (message_size >> 16) & 0xff;
        header[8] = (message_size >> 8) & 0xff;
        header[9] = (message_size >> 0) & 0xff;
        if (m_useMask)
        {
            header[10] = masking_key[0];
            header[11] = masking_key[1];
            header[12] = masking_key[2];
            header[13] = masking_key[3];
        }
    }

    // 发送缓冲在 socket 可写前会持续增长
    std::vector<uint8_t> txbuf;
    txbuf.insert(txbuf.end(), header.begin(), header.end());
    txbuf.insert(txbuf.end(), message_begin, message_end);
    if (m_useMask)
    {
        for (size_t i = 0; i < message_size; ++i)
        {
            *(txbuf.end() - message_size + i) ^= masking_key[i & 0x3];
        }
    }

    send(conn, txbuf);
}

} // namespace ws
} // namespace net
} // namespace afl
