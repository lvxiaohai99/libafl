/**
 * @file   NetTest.cpp
 * @brief  net 模块单元测试：ByteBuffer / InetAddress / EventLoop+Timer /
 *         TcpServer+TcpClient 回环 / HttpServer 冒烟
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

#include "afl/net/ByteBuffer.h"
#include "afl/net/InetAddress.h"
#include "afl/net/EventLoop.h"
#include "afl/net/TcpServer.h"
#include "afl/net/TcpClient.h"
#include "afl/net/http/HttpServer.h"
#include "afl/net/http/HttpResponse.h"
#include "afl/net/http/HttpProtocol.h"

using namespace afl::net;

namespace
{
/// 避免端口冲突：基于 pid 取一个高位端口
uint16_t pickPort(uint16_t base)
{
    return static_cast<uint16_t>(20000 + (base + ::getpid()) % 20000);
}

/// 轮询等待条件成立（毫秒），返回是否成功
bool waitFor(const std::function<bool()>& cond, int timeoutMs)
{
    for (int i = 0; i < timeoutMs / 5; ++i)
    {
        if (cond())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return cond();
}

} // namespace

// ---------------------------------------------------------------------------
// ByteBuffer
// ---------------------------------------------------------------------------
TEST(NetTest, ByteBufferStringReadWrite)
{
    ByteBuffer buf;
    EXPECT_EQ(0u, buf.readableBytes());

    buf.write(std::string("hello"));
    EXPECT_EQ(5u, buf.readableBytes());
    EXPECT_EQ("hello", buf.toString());
    EXPECT_EQ("hello", buf.retrieveAllAsString());
    EXPECT_EQ(0u, buf.readableBytes());
}

TEST(NetTest, ByteBufferNumberNetworkEndian)
{
    ByteBuffer buf;
    const uint32_t hostValue = 0x12345678;
    buf.write(hostValue);

    // 网络字节序写入后读回应还原主机值
    uint32_t back = buf.read<uint32_t>();
    EXPECT_EQ(hostValue, back);

    const int16_t small = -300;
    buf.write(small);
    EXPECT_EQ(small, buf.read<int16_t>());
}

TEST(NetTest, ByteBufferPartialRetrieve)
{
    ByteBuffer buf;
    buf.write(std::string("abcdef"));
    EXPECT_EQ("abc", buf.retrieveAsString(3));
    EXPECT_EQ(3u, buf.readableBytes());
    EXPECT_EQ("def", buf.retrieveAllAsString());
}

// ---------------------------------------------------------------------------
// InetAddress
// ---------------------------------------------------------------------------
TEST(NetTest, InetAddressIpPort)
{
    InetAddress addr("127.0.0.1", 8080);
    EXPECT_EQ("127.0.0.1", addr.ip());
    EXPECT_EQ(8080u, addr.port());

    InetAddress any(9000);
    EXPECT_EQ(9000u, any.port());
}

TEST(NetTest, InetAddressResolve)
{
    InetAddress addr;
    EXPECT_TRUE(InetAddress::resolve("127.0.0.1", &addr));
    EXPECT_EQ("127.0.0.1", addr.ip());
}

// ---------------------------------------------------------------------------
// EventLoop + TimerQueue 冒烟
// ---------------------------------------------------------------------------
TEST(NetTest, EventLoopTimerSmoke)
{
    EventLoop loop;
    std::atomic<int> fired(0);

    // loop 启动前加入 50ms 定时器；回调后退出 loop
    loop.addTimer(
        [&] {
            fired.fetch_add(1);
            loop.quit();
        },
        0.05, false);

    loop.loop();
    EXPECT_EQ(1, fired.load());
}

// ---------------------------------------------------------------------------
// TcpServer + TcpClient 本地回环 echo
// ---------------------------------------------------------------------------
TEST(NetTest, TcpEchoLoopback)
{
    const uint16_t port = pickPort(1);
    EventLoop loop;
    TcpServer server(&loop, InetAddress(port), "EchoServer");

    server.setMessageCallback([](const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp) {
        // 使用 ByteBuffer* 重载：无论是否在 loop 线程，数据都会被拷贝后安全发送
        conn->send(buf);
    });
    server.start();

    std::atomic<bool> connected(false);
    std::atomic<bool> echoDone(false);
    std::string received;

    TcpClient client(&loop, InetAddress("127.0.0.1", port), "EchoClient");
    client.setConnectionCallback([&](const TcpConnectionPtr& conn) {
        if (conn->connected())
        {
            connected = true;
            conn->send("ping-libafl");
        }
    });
    client.setMessageCallback([&](const TcpConnectionPtr&, ByteBuffer* buf, TimeStamp) {
        received = buf->retrieveAllAsString();
        echoDone = true;
    });
    client.connect();

    std::thread loopThread([&] { loop.loop(); });

    ASSERT_TRUE(waitFor([&] { return echoDone.load(); }, 3000))
        << "echo not received, connected=" << connected.load();
    EXPECT_EQ("ping-libafl", received);

    loop.quit();
    loopThread.join();
}

// ---------------------------------------------------------------------------
// HttpServer + 裸 socket GET 冒烟
// ---------------------------------------------------------------------------
TEST(NetTest, HttpServerGetSmoke)
{
    const uint16_t port = pickPort(2);
    EventLoop loop;
    HttpServer server(&loop, InetAddress(port), "HttpUtServer");

    server.setCallback(HttpGet, [](const HttpRequest&, HttpResponse* resp) {
        resp->setStatusCode(HttpStatusOk);
        resp->setBody("libafl-http-ok");
        resp->setCloseConnection(true);
    });
    server.start();

    std::thread loopThread([&] { loop.loop(); });
    // 等服务器监听就绪
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 裸 socket 发送 GET 请求
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ASSERT_EQ(0, ::connect(fd, reinterpret_cast<struct sockaddr*>(&servAddr), sizeof(servAddr)));

    const char* request = "GET /hello HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    ASSERT_GT(static_cast<long>(::send(fd, request, strlen(request), 0)), 0L);

    std::string response;
    char chunk[1024];
    for (;;)
    {
        ssize_t n = ::recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0)
            break;
        response.append(chunk, static_cast<size_t>(n));
        if (response.find("libafl-http-ok") != std::string::npos &&
            response.find("\r\n\r\n") != std::string::npos)
        {
            // body 已到（Content-Length 固定）
            break;
        }
    }
    ::close(fd);

    EXPECT_TRUE(response.find("200") != std::string::npos)
        << "response head: " << response.substr(0, 64);
    EXPECT_TRUE(response.find("libafl-http-ok") != std::string::npos)
        << "body not found in response";

    loop.quit();
    loopThread.join();
}

#include "afl/net/websocket/WebSocket.h"

TEST(NetTest, WebSocketEncodeTextFrame)
{
    using namespace afl::net::ws;
    const char* msg = "hello";
    char out[256];
    int n = encodeFrame(WS_TEXT_FRAME, msg, 5, out, sizeof(out));
    EXPECT_GT(n, 5);
    // FIN + TEXT opcode
    EXPECT_EQ(static_cast<unsigned char>(WS_TEXT_FRAME), static_cast<unsigned char>(out[0]));
}

#ifdef AFL_ENABLE_SSL
#include "afl/net/SslHelper.h"
TEST(NetTest, SslHelperConstructAndMissingCert)
{
    afl::net::SslHelper helper;
    helper.initSsl();
    afl::net::SslConnection conn;
    afl::net::SslCertPaths cert;
    cert.caCertificateFile = "/tmp/libafl_no_such_ca.pem";
    cert.clientCertificateFile = "/tmp/libafl_no_such_cli.pem";
    cert.clientPrivateKeyFile = "/tmp/libafl_no_such_key.pem";
    EXPECT_FALSE(helper.createVerifyContext(&conn, cert));
}
#endif
