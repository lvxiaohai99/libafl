/**
 * @file   HttpContext.h
 * @brief  HTTP 协议解析上下文
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/net/TcpServer.h"
#include "afl/net/CallBacks.h"
#include "afl/net/http/HttpProtocol.h"
#include "afl/net/http/HttpRequest.h"
#include "afl/time/TimeStamp.h"
using afl::time::TimeStamp;
namespace afl
{
namespace net
{
class EventLoop;
class HttpRequest;
class HttpResponse;
class InetAddress;
class ByteBuffer;

/** @brief 流式 HTTP 请求解析状态机 */
class HttpContext
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext() : m_state(kExpectRequestLine) {}

public:
    bool parseRequest(ByteBuffer* buf, TimeStamp receiveTime);

public:
    bool expectRequestLine() const { return m_state == kExpectRequestLine; }
    bool expectHeaders() const { return m_state == kExpectHeaders; }
    bool expectBody() const { return m_state == kExpectBody; }
    bool gotAll() const { return m_state == kGotAll; }
    void receiveRequestLine() { m_state = kExpectHeaders; }
    void receiveHeaders()
    {
        if (m_request.method() == HttpGet)
            m_state = kGotAll;
        else if (m_request.method() == HttpPost)
            m_state = kExpectBody;
    }
    void receiveBody() { m_state = kGotAll; }

    HttpRequest& request() { return m_request; }
    const HttpRequest& request() const { return m_request; }

    void reset()
    {
        m_state = kExpectRequestLine;
        HttpRequest dummy;
        m_request.swap(dummy);
    }

private:
    bool processRequestLine(const char* begin, const char* end);
    bool processReqestHeader(const char* begin, const char* end);

private:
    HttpRequestParseState m_state;
    HttpRequest m_request;
};

} // namespace net
} // namespace afl
