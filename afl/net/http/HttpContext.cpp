/**
 * @file   HttpContext.cpp
 * @brief  HTTP 协议解析上下文的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/http/HttpContext.h"
#include "afl/net/ByteBuffer.h"
#include "afl/time/TimeStamp.h"
#include "afl/string/StringUtil.h"

using afl::time::TimeStamp;

namespace afl
{
namespace net
{
/** 报文结构：请求行 → 头 → 空行 → 可选 body */
// 示例：浏览器访问 127.0.0.1:8888/index.html 时服务端可能收到的报文片段
// GET /index.html HTTP/1.1
// Host: 127.0.0.1:8888
// Connection: keep-alive
// Cache-Control: max-age=0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*; q = 0.8
// User - Agent: Mozilla / 5.0 (Windows NT 6.1; WOW64) AppleWebKit / 537.36 (KHTML, like
// Gecko) Chrome / 35.0.1916.153 Safari / 537.36
// Accept - Encoding : gzip, deflate, sdch
// Accept - Language : zh - CN, zh; q = 0.8
// RA - Ver: 2.2.22
// RA - Sid : 7B747245 - 20140622 - 042030 - f79ea7 - 5f07a8
bool HttpContext::parseRequest(ByteBuffer* buf, TimeStamp receiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore)
    {
        if (this->expectRequestLine())
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                ok = processRequestLine(buf->peek(), crlf); // 解析请求
                if (ok)
                {
                    this->request().setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);
                    this->receiveRequestLine(); // 请求行解析完成，下一步要解析消息头中的参
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (this->expectHeaders()) // 解析消息头中的参
        {
            //printf("context->expectHeaders() [%p]\n", this);
            const char* crlf = buf->findCRLF();
            if (crlf) //按行添加消息头中的参
            {
                //const char *colon = std::find(buf->peek(), crlf, ':'); //行一行遍
                if (!processReqestHeader(buf->peek(), crlf)) // 消息头解析完
                {
                    // 空行，消息头结束
                    this->receiveHeaders(); //下一步应该按get/post来区分是否解析消息体
                    hasMore = !this->gotAll();
                    //printf("parse headers [%d][%d]\n", hasMore, m_state);
                    //const map<string, string>& headers = this->request().headers();
                    //for(map<string, string>::const_iterator it = headers.begin(); it!=headers.end(); ++it)
                    //    printf("HttpContext::parseRequest headers [%s = %s]\n", it->first.c_str(), it->second.c_str());
                }
                buf->retrieveUntil(crlf + 2);
                //printf("context->expectHeaders() [%s]", buf->toString().c_str());
                //if(this->expectBody())   // 如果是解析header完成，且要解析body，过滤掉下面的空白行（\r\n
                //     buf->retrieve(2);
            }
            else
            {
                hasMore = false;
            }
        }
        else if (this->expectBody()) // 解析 body // FIXME：
        {
            int bufsize = static_cast<int>(buf->readableBytes());
            string value = m_request.getHeader("Content-Length");
            assert(!value.empty());
            int content_len = afl::str::strTo<int>(value);
            printf("context->expectBody() [%p][%d][%d]\n", this, bufsize, content_len);
            printf("context->expectBody() [%s]", buf->toString().c_str());

            if (bufsize >= content_len)
            {
                this->receiveBody();
                assert(gotAll());
                m_request.setBody(buf->retrieveAsString(content_len));
                printf("parse all data from request[%d]", m_state);
                hasMore = false;
            }
            else // FIXME：对端 body 不完整或停发时会卡住
            {
                break;
            }
        }
        else // 已全部接收
        {
            assert(0 && "不应再进入接收逻辑");
        }
    }
    return ok;
}

/** 请求行：Method Path HTTP/x.x */
bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    HttpRequest& request = this->request();
    string method(start, space);
    if (space != end && request.setMethod(method))
    {
        // 解析 URL 路径与 query
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                string u(start, question);
                request.setPath(u);
                u.assign(question, space);
                request.setQuery(u);
            }
            else
            {
                string u(start, question);
                request.setPath(u);
            }

            // 解析 HTTP 版本
            start = space + 1;
            if (end - start != 8) // 非 HTTP/1.0/1.1
            {
                return false;
            }

            if (std::equal(start, end, "HTTP/1.1"))
                request.setVersion(HTTP_VERSION_1_1);
            else if (std::equal(start, end, "HTTP/1.0"))
                request.setVersion(HTTP_VERSION_1_0);

            return true;
        }
    }
    return false;
}

/** 解析单行消息头 field: value */
bool HttpContext::processReqestHeader(const char* begin, const char* end)
{
    // TODO：按需改为循环解析多行
    {
        const char* colon = std::find(begin, end, ':'); //行一行遍
        if (colon != end)
        {
            string field(begin, colon);
            ++colon;
            while (colon < end && isspace(*colon))
            {
                ++colon;
            }
            string value(colon, end);
            while (!value.empty() && isspace(value[value.size() - 1]))
            {
                value.resize(value.size() - 1);
            }
            m_request.addHeader(field, value);
            return true;
        }
        else
        {
            // 空行，消息头结束
            return false;
        }
    }
}

} // namespace net
} // namespace afl
