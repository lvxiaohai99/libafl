/**
 * @file   HttpRequest.cpp
 * @brief  HTTP 请求的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/http/HttpRequest.h"
#include <string.h>
#include <algorithm>
namespace afl
{
namespace net
{
/** 请求行：Method Path HTTP/x.x */
static bool processRequestLine(const char* begin, const char* end, HttpRequest* req)
{
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    string method(start, space);
    if (space != end && req->setMethod(method))
    {
        // 解析路径与 query
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                string u(start, question);
                req->setPath(u);
                u.assign(++question, space); //不包?
                req->setQuery(u);
            }
            else
            {
                string u(start, question);
                req->setPath(u);
            }

            // 解析 HTTP 版本
            start = space + 1;
            if (end - start != 8) // 非 HTTP/1.0/1.1
            {
                return false;
            }

            if (std::equal(start, end, "HTTP/1.1"))
                req->setVersion(HTTP_VERSION_1_1);
            else if (std::equal(start, end, "HTTP/1.0"))
                req->setVersion(HTTP_VERSION_1_0);

            return true;
        }
    }
    return false;
}

static bool processRequestHeaders(const char* begin, const char* end, HttpRequest* req)
{
    const char* nextCRLF = strstr(begin, CRLF);
    while (nextCRLF && nextCRLF < end)
    {
        const char* colon = std::find(begin, nextCRLF, ':'); //行一行遍
        if (colon != nextCRLF)
        {
            string field(begin, colon);
            ++colon;
            while (colon < nextCRLF && isspace(*colon))
            {
                ++colon;
            }
            string value(colon, nextCRLF);
            while (!value.empty() && isspace(value[value.size() - 1]))
            {
                value.resize(value.size() - 1);
            }
            req->addHeader(field, value);
        }
        else
        {
            // 空行，消息头结束
            return true;
        }

        begin = nextCRLF + 2;
        nextCRLF = strstr(begin, CRLF);
    }
    return true;
}

/*static*/ bool HttpRequest::parseRequest(const char* requestLineandHeader, size_t len,
                                          HttpRequest* req)
{
    //解析Http消息头的第一行，即请求行，Method Location HttpVer  GET /index.html HTTP/1.1
    const char* start = requestLineandHeader;
    const char* firstCRLF = strstr(start, CRLF);
    if (!firstCRLF)
        return false;

    if (!processRequestLine(start, firstCRLF, req))
        return false;

    return processRequestHeaders(firstCRLF + 2, requestLineandHeader + len, req);
}

} // namespace net
} // namespace afl
