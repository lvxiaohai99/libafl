/**
 * @file   HttpResponse.h
 * @brief  HTTP 响应封装
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"
#include "afl/net/http/HttpProtocol.h"

#include <map>
#include <string>

namespace afl
{
namespace net
{
class ByteBuffer;
class HttpRequest;

/**
 * @brief HTTP 响应：状态码、头、正文
 */
class HttpResponse
{
public:
    /**
     * @brief 构造
     * @param closeConn 是否在响应后关闭连接
     */
    explicit HttpResponse(bool closeConn = true);
    ~HttpResponse();

    /** @brief 设置状态码 @param code 状态码 */
    void setStatusCode(HttpStatusCode code) { m_statusCode = code; }
    /** @brief 设置 HTTP 版本 @param ver 版本 */
    void setVersion(HttpVersion ver) { m_version = ver; }
    /** @brief 设置 Server 头 @param name 服务名 */
    void setServerName(const std::string& name) { m_serverName = name; }
    /** @brief 设置 Content-Type @param type MIME */
    void setContentType(const std::string& type) { m_contentType = type; }
    /** @brief 设置正文 @param body 正文 */
    void setBody(const std::string& body) { m_body = body; }
    /** @brief 获取正文 */
    const std::string& body() const { return m_body; }
    /** @brief 添加响应头 @param key 名 @param value 值 */
    void addHeader(const std::string& key, const std::string& value) { m_headers[key] = value; }
    /** @brief 是否在响应后关闭连接 @param on true 关闭 */
    void setCloseConnection(bool on) { m_closeConnection = on; }

    /**
     * @brief 序列化到缓冲区
     * @param output 输出缓冲
     */
    void compileToBuffer(ByteBuffer* output) const;

    /** @brief 是否关闭连接 */
    bool closeConnection() const { return m_closeConnection; }

private:
    HttpStatusCode m_statusCode;
    HttpVersion m_version;
    bool m_closeConnection;
    std::string m_serverName;
    std::string m_contentType;
    std::string m_body;
    std::map<std::string, std::string> m_headers;
};

} // namespace net
} // namespace afl
