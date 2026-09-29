/**
 * @file   HttpKeyValue.h
 * @brief  HTTP 状态码、Content-Type、Method 字符串映射
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Singleton.h"
#include "afl/base/Common.h"
#include "afl/net/http/HttpProtocol.h"

namespace afl
{
namespace net
{
class HttpKeyValue : public afl::base::Singleton<HttpKeyValue>
{
    DECLARE_SINGLETON_CLASS(HttpKeyValue);

public:
    std::string getStatusDesc(HttpStatusCode code) const;
    std::string getContentType(const std::string& file_type) const;
    std::string getMethodStr(HttpMethod method) const;

private:
    std::map<HttpStatusCode, std::string> m_codeDesc;
    std::map<std::string, std::string> m_contentType;
    std::map<HttpMethod, std::string> m_methodStr;

private:
    HttpKeyValue();
    void initialise();
};

} // namespace net
} // namespace afl
