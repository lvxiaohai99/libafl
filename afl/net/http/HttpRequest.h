/**
 * @file   HttpRequest.h
 * @brief  HTTP 请求封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include "afl/net/http/HttpProtocol.h"
#include "afl/net/http/HttpKeyValue.h"
#include "afl/string/StringUtil.h"

using afl::time::TimeStamp;
namespace afl
{
namespace net
{
/** @brief HTTP 请求（方法、路径、头、body） */
class HttpRequest
{
public:
    /** @brief 消息头 map，键比较忽略大小写 */
    typedef std::map<std::string, std::string, afl::str::StringCmpNocase> HeadersMap;
    static bool parseRequest(const char* requestLineandHeader, size_t len, HttpRequest* req);

public:
    HttpRequest() : m_method(HttpInvalid), m_version(HTTP_VERSION_0_0) {}

    ~HttpRequest() {}

public:
    void setMethod(HttpMethod method) { m_method = method; }
    bool setMethod(const std::string& method)
    {
        if (method == "GET")
            m_method = HttpGet;
        else if (method == "POST")
            m_method = HttpPost;
        else if (method == "HEAD")
            m_method = HttpHead;
        else if (method == "PUT")
            m_method = HttpPut;
        else if (method == "DELETE")
            m_method = HttpDelete;
        else
            m_method = HttpInvalid;
        return m_method != HttpInvalid;
    }
    HttpMethod method() const { return m_method; }
    const std::string methodStr() const
    {
        return HttpKeyValue::getInstanceRef().getMethodStr(m_method);
    }

    void setVersion(HttpVersion httpver) { m_version = httpver; }
    void setVersion(const std::string& httpver)
    {
        m_version = (httpver == "HTTP/1.1" ? HTTP_VERSION_1_1 : HTTP_VERSION_1_0);
    }
    HttpVersion version() const { return m_version; }
    const char* versionStr() const
    {
        return m_version == HTTP_VERSION_1_1 ? "HTTP/1.1" : "HTTP/1.0";
    }

    void setPath(const std::string& url) { m_urlpath = url; }
    const std::string& path() const { return m_urlpath; }

    void setQuery(const std::string& url) { m_query = url; }
    const std::string& query() const { return m_query; }

    void setBody(const std::string& body) { m_body = body; }
    const std::string& body() const { return m_body; }

    void setReceiveTime(TimeStamp t) { m_receiveTime = t; }
    TimeStamp receiveTime() const { return m_receiveTime; }

    void addHeader(const std::string& key, const std::string& value) { m_headers[key] = value; }
    std::string getHeader(const std::string& field) const
    {
        std::string result;
        HeadersMap::const_iterator it = m_headers.find(field);
        if (it != m_headers.end())
        {
            result = it->second;
        }
        return result;
    }
    const HeadersMap& headers() const { return m_headers; }

    void swap(HttpRequest& that)
    {
        std::swap(m_method, that.m_method);
        std::swap(m_version, that.m_version);
        m_urlpath.swap(that.m_urlpath);
        m_query.swap(that.m_query);
        m_receiveTime.swap(that.m_receiveTime);
        m_headers.swap(that.m_headers);
    }

    std::string dump() const
    {
        std::string result;
        result =
            std::string(methodStr()) + " " + path() + "?" + query() + " " + versionStr() + "\r\n";
        for (HeadersMap::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
        {
            result += it->first;
            result += ": ";
            result += it->second;
            result += "\r\n";
        }
        return result;
    }

private:
    HttpMethod m_method;
    HttpVersion m_version;
    std::string m_urlpath; // url
    std::string m_query;   // url后面的以?分割的参(不包?)
    std::string m_body;
    TimeStamp m_receiveTime;

    HeadersMap m_headers;
};

} // namespace net
} // namespace afl
