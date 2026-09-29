/**
 * @file   WebSocketClient.cpp
 * @brief  WebSocket 客户端的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/websocket/WebSocketClient.h"
#include "afl/base/Common.h"
#include "afl/log/Log.h"
#include "afl/net/TcpClient.h"
#include "afl/net/TcpConnection.h"
#include "afl/net/http/HttpContext.h"
#include "afl/net/http/HttpRequest.h"
#include "afl/net/http/HttpResponse.h"
#include "afl/net/websocket/WebSocket.h"
#include "afl/string/StringUtil.h"
namespace afl
{
namespace net
{
namespace ws
{
const char* const kWebSocketMagicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char* const kSecWebSocketKeyHeader = "Sec-WebSocket-Key";
const char* const kSecWebSocketVersionHeader = "Sec-WebSocket-Version";
const char* const kUpgradeHeader = "Upgrade";
const char* const kConnectionHeader = "Connection";
const char* const kSecWebSocketProtocolHeader = "Sec-WebSocket-Protocol";
const char* const kSecWebSocketAccept = "Sec-WebSocket-Accept";

WsClient::WsClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& url,
                   const string& cilentname /* = "WsClient"*/)
    : m_onopen(NULL), m_onclose(NULL), m_onmessage(NULL), m_url(url), m_useMask(true), m_conn(NULL),
      m_serverAddr(serverAddr)
{
    m_client = new afl::net::TcpClient(loop, serverAddr, cilentname);
    m_client->setConnectionCallback(
        std::bind(&WsClient::onConnection, this, std::placeholders::_1));
    m_client->setMessageCallback(std::bind(&WsClient::onMessage, this, std::placeholders::_1,
                                           std::placeholders::_2, std::placeholders::_3));
}

WsClient::~WsClient()
{
    delete m_client;
}

void WsClient::setCustomOption(const std::string op)
{
    size_t sz = op.size();
    std::string nop = op;

    if (sz < 2)
        return;

    if (op[sz - 2] != '\r' || op[sz - 1] != '\n')
        nop.append("\r\n");

    m_customOptions.push_back(nop);
}

void WsClient::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO("WsClient::onConnection get one client %d", conn->fd());
    if (conn->connected())
    {
        conn->setContext(WsConnection());
        sendHandshake(conn); /// 连接成功之后就开始握
        m_conn = conn;
    }
    else
    {
        if (m_onclose)
            m_onclose(conn);
    }
}

/** @brief FIXME：服务端报文解析宜独立成解码器，持久化中间状态，避免重复解析 */
void WsClient::onMessage(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime)
{
    LOG_INFO("WsClient::onMessage recv data (fd =%d)(size = %d)", conn->fd(), buf->readableBytes());
    WsConnection* wsconn = afl::base::any_cast<WsConnection>(conn->getMutableContext());
    assert(wsconn);
    if (!wsconn->handshaked()) /// 尚未握手
    {
        // 要先解析 server端返回的握手信息
        int ret = parseHandshakeResponse(conn, buf);
        if (ret == 0) /// 握手响应已完整且合法
        {
            wsconn->setHandshaked(true);
            wsconn->setConnState(WS_CONN_OPEN);
            if (m_onopen)
            {
                m_onopen(conn);
            }
        }
        else if (ret < 0)
        {
            LOG_WARN("parseHandshakeResponse failure, close it(%d)", ret);
            conn->shutdown();
            m_client->reconnect();
        }

        return;
    }

    while (1) /// 已经握手，接下来就是正常收发数据
    {
        if (buf->readableBytes() < 2) /// WebSocket 帧头至少 2 字节
        {
            return;
        }
        WsHeader ws;
        const uint8_t* data = (const uint8_t*)buf->peek(); // 只窥视，不消费
        ws.fin = (data[0] & 0x80) == 0x80;
        ws.opcode = (WsOpcode)(data[0] & 0x0f);
        ws.mask = (data[1] & 0x80) == 0x80;
        ws.N0 = (data[1] & 0x7f);
        ws.header_size = 2 + (ws.N0 == 126 ? 2 : 0) + (ws.N0 == 127 ? 8 : 0) + (ws.mask ? 4 : 0);
        int i = 0;
        if (ws.N0 < 126)
        {
            ws.N = ws.N0;
            i = 2;
        }
        else if (ws.N0 == 126)
        {
            ws.N = 0;
            ws.N |= ((uint64_t)data[2]) << 8;
            ws.N |= ((uint64_t)data[3]) << 0;
            i = 4;
        }
        else if (ws.N0 == 127)
        {
            ws.N = 0;
            ws.N |= ((uint64_t)data[2]) << 56;
            ws.N |= ((uint64_t)data[3]) << 48;
            ws.N |= ((uint64_t)data[4]) << 40;
            ws.N |= ((uint64_t)data[5]) << 32;
            ws.N |= ((uint64_t)data[6]) << 24;
            ws.N |= ((uint64_t)data[7]) << 16;
            ws.N |= ((uint64_t)data[8]) << 8;
            ws.N |= ((uint64_t)data[9]) << 0;
            i = 10;
        }
        if (ws.mask)
        {
            ws.masking_key[0] = ((uint8_t)data[i + 0]) << 0;
            ws.masking_key[1] = ((uint8_t)data[i + 1]) << 0;
            ws.masking_key[2] = ((uint8_t)data[i + 2]) << 0;
            ws.masking_key[3] = ((uint8_t)data[i + 3]) << 0;
        }
        else
        {
            ws.masking_key[0] = 0;
            ws.masking_key[1] = 0;
            ws.masking_key[2] = 0;
            ws.masking_key[3] = 0;
        }
        LOG_DEBUG("WsHeader: fin=%d, opcode=%d, mask=%d, N0=%d, N=%d, header_size=%d", ws.fin,
                  ws.opcode, ws.mask, ws.N0, ws.N, ws.header_size);

        // 帧头与载荷长度已齐，开始处理
        if (buf->readableBytes() < ws.header_size + ws.N) /// 实际数据还未到达
        {
            return;
        }

        std::vector<char> rxbuf;
        rxbuf.reserve(ws.N);
        if (false) {}
        else if (ws.opcode == WS_OPCODE_PONG)
        {
        }
        else if (ws.opcode == WS_OPCODE_CLOSE)
        {
            close(conn);
        }
        else if (ws.opcode == WS_OPCODE_PING)
        {
            rxbuf.assign(buf->peek(), buf->peek() + ws.N);
            if (ws.mask)
            {
                for (size_t i = 0; i < ws.N; ++i)
                {
                    rxbuf[i] ^= ws.masking_key[i & 0x3];
                }
            }
            sendData(conn, WS_OPCODE_PONG, rxbuf.size(), rxbuf.begin(), rxbuf.end());
        }
        else if (ws.opcode == WS_OPCODE_TEXT || ws.opcode == WS_OPCODE_BINARY ||
                 ws.opcode == WS_OPCODE_CONTINUE)
        {
            LOG_INFO("========= (%d)(%d)(%d)(%s)", ws.header_size, ws.N, buf->readableBytes(),
                     buf->toString().c_str());
            rxbuf.assign(buf->peek() + ws.header_size, buf->peek() + ws.header_size + ws.N);
            LOG_INFO("111111 : %s", rxbuf.data());
            if (ws.mask)
            {
                for (size_t i = 0; i < ws.N; ++i)
                {
                    rxbuf[i] ^= ws.masking_key[i & 0x3];
                }
            }
            if (ws.fin && m_onmessage)
            {
                m_onmessage(conn, rxbuf, afl::time::TimeStamp::now());
            }
        }
        else
        {
            LOG_ERROR("收到未预期的 WebSocket 帧类型\n");
            close(conn);
        }
        buf->retrieve(ws.header_size + ws.N);
        LOG_INFO("========= (%s)(%d)", buf->toString().c_str(), buf->readableBytes());
    }
}

/// 发websocket握手请求
void WsClient::sendHandshake(const TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        return;
    }
    std::string buffer = makeHandshakeRequest(m_url);
    afl::str::stringFormatAppend(&buffer, "Host: %s:%d\r\n", m_serverAddr.ip().c_str(),
                                 m_serverAddr.port());

    for (auto& op : m_customOptions)
        buffer.append(op);

    buffer.append("\r\n");

    LOG_DEBUG("client[%d] request sendHandshake : (%s)\n", conn->fd(), buffer.c_str());
    conn->send(buffer);
}

int WsClient::parseHandshakeResponse(const TcpConnectionPtr& conn, ByteBuffer* buf)
{
    const char* doubleCRLF = buf->findDoubleCRLF();
    if (doubleCRLF == NULL)
    {
        if (buf->readableBytes() > 1024 * 1024) /// 超过 1MB 仍找不到 "\r\n\r\n"
        {
            LOG_ERROR("Cannot find the double crlf in server handshake Response");
            return -1;
        }
    }
    else
    {
        /// TODO：校验 Sec-WebSocket-Accept 等字段，确认服务端握手成功
        /***
            HTTP/1.1 101 Switching Protocols
            Upgrade: WebSocket
            Connection: Upgrade
            Sec-WebSocket-Version: 13
            Sec-WebSocket-Accept: tlfzFb2mOM86dj/ZWpxF0VCvy6s=

        ***/
        const std::string Response(buf->peek(), doubleCRLF + 4);
        if (strcasestr(Response.c_str(), "HTTP/1.1 101") == NULL ||
            strcasestr(Response.c_str(), "Upgrade: WebSocket") == NULL ||
            strcasestr(Response.c_str(), "Connection: Upgrade") == NULL)
        {
            LOG_ERROR("server handshake Response is invalid(%s)", Response.c_str());
            return -1;
        }

        buf->retrieve(doubleCRLF + 4 - buf->peek());
        //LOG_ALERT("%d, %d, %d", buf->readableBytes(), Response.size(), doubleCRLF - buf->peek());
        LOG_INFO("WsClient::onMessage  parse request over.");
        LOG_ALERT("####### %d", buf->readableBytes());
        return 0;
    }
    return 1;
}

void WsClient::connect()
{
    m_client->enableRetry();
    m_client->connect();
}

void WsClient::close(const TcpConnectionPtr& conn, WsCloseReason code, const char* reason)
{
    uint8_t closeFrame[6] = {0x88, 0x80, 0x00, 0x00, 0x00, 0x00}; // 后 4 字节为掩码键
    std::vector<uint8_t> header(closeFrame, closeFrame + 6);
    std::vector<uint8_t> txbuf;
    txbuf.insert(txbuf.end(), header.begin(), header.end());
    send(conn, txbuf);

    if (m_onclose)
        m_onclose(conn);

    m_client->reconnect();
}

void WsClient::send(const TcpConnectionPtr& conn, const std::vector<uint8_t>& data)
{
    conn->send(&*data.begin(), data.size());
}

} // namespace ws
} // namespace net
} // namespace afl
