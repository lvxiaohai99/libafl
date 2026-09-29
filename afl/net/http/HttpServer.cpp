/**
 * @file   HttpServer.cpp
 * @brief  HTTP 服务器的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/log/Log.h"
#include "afl/net/http/HttpServer.h"
#include "afl/net/TcpConnection.h"
#include "afl/net/http/HttpContext.h"
#include "afl/net/http/HttpRequest.h"
#include "afl/net/http/HttpResponse.h"

using namespace afl::base;
namespace afl
{
namespace net
{
void defaultHttpCallback(const HttpRequest& req, HttpResponse* resp)
{
    resp->setStatusCode(HttpStatusOk);
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr,
                       const string& servername /* = "HttpServer"*/)
    : TcpServer(loop, listenAddr, servername)
{
    setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1,
                                 std::placeholders::_2, std::placeholders::_3));
}

HttpServer::~HttpServer() {}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO("HttpServer::onConnection get one client %d", conn->fd());
    if (conn->connected())
    {
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime)
{
    LOG_INFO("HttpServer::onConnection recv data [%d][%s]", conn->fd(), buf->toString().c_str());

    HttpContext* context = afl::base::any_cast<HttpContext>(conn->getMutableContext());
    assert(context);

    while (1)
    {
        if (!context->parseRequest(buf, receiveTime))
        {
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }

        if (context->gotAll())
        {
            LOG_INFO("HttpServer::onMessage  parse request over.");
            Response(conn, context->request());
            context
                ->reset(); // 处理完响应后 reset 上下文，便于 keep-alive
        }
        else
            break;
    }
}

void HttpServer::Response(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    const string& connection = req.getHeader("Connection");
    bool close =
        connection == "close" || (req.version() == HTTP_VERSION_1_0 && connection != "Keep-Alive");

    HttpResponse Response(close);
    Response.setStatusCode(HttpStatusOk);
    Response.setServerName("MyHttpServer");

    methodCallback(req, &Response); // 回调填充 Response

    ByteBuffer buf;
    Response.compileToBuffer(&buf);
    //printf("[%s]\n", buf.toString().c_str());
    conn->send(&buf);

    LOG_INFO("HttpServer::Response send data [%d]", conn->fd());
    if (Response.closeConnection())
    {
        LOG_INFO("HttpServer::Response close this[%d]", conn->fd());
        conn->shutdown();
    }
}

void HttpServer::methodCallback(const HttpRequest& req, HttpResponse* resp)
{
    HttpMethod m = req.method();
    map<HttpMethod, HttpCallback>::iterator it = m_methodCallback.find(m);
    if (it != m_methodCallback.end())
    {
        assert(it->second);
        (it->second)(req, resp);
    }
}

} // namespace net
} // namespace afl
