/**
 * @file   HttpResponse.cpp
 * @brief  HTTP 响应的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/http/HttpResponse.h"
#include "afl/net/http/HttpKeyValue.h"
#include "afl/file/FileUtil.h"
#include "afl/net/ByteBuffer.h"
namespace afl
{
namespace net
{
HttpResponse::HttpResponse(bool closeConn /* = true*/)
    : m_statusCode(HttpStatusOk), m_closeConnection(closeConn)
{
    m_version = HTTP_VERSION_0_0;
}
HttpResponse::~HttpResponse() {}

/** 响应格式：
/// 状态行 \r\n
/// 响应头 \r\n
/// 空行 \r\n
/// 响应体 */
void HttpResponse::compileToBuffer(ByteBuffer* output) const
{
    HttpKeyValue* ptable = HttpKeyValue::getInstancePtr();

    // 状态行：版本 / 状态码 / 原因短语
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", m_statusCode,
             ptable->getStatusDesc(m_statusCode).c_str());

    output->write(buf, strlen(buf));

    // 响应头
    if (!m_serverName.empty())
    {
        output->write("Server: ");
        output->write(m_serverName);
        output->write("\r\n");
    }
    if (!m_contentType.empty())
    {
        output->write("Content-Type: ");
        output->write(m_contentType);
        output->write("\r\n");
    }

    if (m_closeConnection)
    {
        output->write("Connection: close\r\n");
    }
    else
    {
        output->write("Connection: Keep-Alive\r\n");
    }

    for (std::map<string, string>::const_iterator it = m_headers.begin(); it != m_headers.end();
         ++it)
    {
        if (it->second.empty())
            continue;
        output->write(it->first);
        output->write(": ");
        output->write(it->second);
        output->write("\r\n");
    }

    //即使没有body，也需要Content-Length: 0，否则由于HTTP 1.1 keep-alive，客户端不知道这个http response是否结束
    memset(buf, 0, 128);
    snprintf(buf, sizeof(buf), "Content-Length: %d\r\n", static_cast<int>(m_body.size()));
    output->write(buf, strlen(buf));

    output->write("\r\n"); //header结束符

    // 响应体（可选）
    if (!m_body.empty())
    {
        output->write(m_body);
    }

    //const string& s =  output->toString();
    //printf("[[%d][\n%s]]\n", (int)s.size(), s.c_str());
}

} // namespace net
} // namespace afl
