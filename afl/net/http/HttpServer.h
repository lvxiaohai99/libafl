/**
 * @file   HttpServer.h
 * @brief  HTTP 服务器
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/TcpServer.h"
#include "afl/net/CallBacks.h"
#include "afl/net/http/HttpProtocol.h"

namespace afl
{
namespace net
{
class EventLoop;
class HttpRequest;
class HttpResponse;
class InetAddress;

/** @brief 基于 TcpServer 的简单 HTTP 服务 */
class HttpServer : public TcpServer
{
public:
    typedef std::function<void(const HttpRequest&, HttpResponse*)> HttpCallback;

public:
    HttpServer(EventLoop* loop, const InetAddress& listenAddr,
               const std::string& servername = "HttpServer");
    ~HttpServer();

public:
    void setRootDir(const std::string& dir) { m_docRootDir = dir; }
    std::string rootDir() { return m_docRootDir; }

    void setDefaultPage(const std::string& page) { m_defaultPage = page; }
    std::string defaultPage() { return m_defaultPage; }

    void setCallback(HttpMethod m, const HttpCallback& cb) { m_methodCallback[m] = cb; }

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, ByteBuffer* buf, TimeStamp receiveTime);
    void Response(const TcpConnectionPtr& conn, const HttpRequest& req);
    void methodCallback(const HttpRequest& req, HttpResponse* resp);

private:
    HttpServer(const HttpServer&);
    HttpServer& operator=(const HttpServer&);

private:
    std::string m_docRootDir;
    std::string m_defaultPage;
    std::map<HttpMethod, HttpCallback> m_methodCallback;
};

} // namespace net
} // namespace afl
