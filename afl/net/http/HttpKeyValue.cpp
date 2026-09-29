/**
 * @file   HttpKeyValue.cpp
 * @brief  HTTP 键值对处理的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/http/HttpKeyValue.h"
namespace afl
{
namespace net
{
HttpKeyValue::HttpKeyValue()
{
    initialise();
}

std::string HttpKeyValue::getStatusDesc(HttpStatusCode code) const
{
    auto iter = m_codeDesc.find(code);
    return iter != m_codeDesc.end() ? iter->second : "";
}

std::string HttpKeyValue::getContentType(const std::string& file_type) const
{
    auto iter = m_contentType.find(file_type);
    return iter != m_contentType.end() ? iter->second : "";
}

std::string HttpKeyValue::getMethodStr(HttpMethod method) const
{
    auto iter = m_methodStr.find(method);
    return iter != m_methodStr.end() ? iter->second : "Invalid";
}

void HttpKeyValue::initialise()
{
    m_contentType["htm"] = "text/html";
    m_contentType["html"] = "text/html";
    m_contentType["css"] = "text/css";
    m_contentType["txt"] = "text/plain";
    m_contentType["c"] = "text/plain";
    m_contentType["cpp"] = "text/plain";
    m_contentType["cxx"] = "text/plain";
    m_contentType["h"] = "text/plain";
    m_contentType["jpeg"] = "image/jpeg";
    m_contentType["jpg"] = "image/jpeg";
    m_contentType["png"] = "image/png";
    m_contentType["bmp"] = "image/bmp";
    m_contentType["gif"] = "image/gif";
    m_contentType["ico"] = "image/x-icon";
    m_contentType["mpg"] = "video/mpeg";
    m_contentType["asf"] = "video/x-ms-asf";
    m_contentType["avi"] = "video/x-msvideo";
    m_contentType["doc"] = "application/msword";
    m_contentType["exe"] = "application/octet-stream";
    m_contentType["rar"] = "application/octet-stream";
    m_contentType["zip"] = "application/zip";
    m_contentType["*"] = "application/octet-stream";

    m_methodStr[HttpHead] = AFL_HTTPMETHOD_HEAD_S;
    m_methodStr[HttpGet] = AFL_HTTPMETHOD_GET_S;
    m_methodStr[HttpPost] = AFL_HTTPMETHOD_POST_S;
    m_methodStr[HttpPut] = AFL_HTTPMETHOD_PUT_S;
    m_methodStr[HttpDelete] = AFL_HTTPMETHOD_DELETE_S;
    m_methodStr[HttpTrace] = AFL_HTTPMETHOD_TRACE_S;
    m_methodStr[HttpOptions] = AFL_HTTPMETHOD_OPTIONS_S;
    m_methodStr[HttpConnect] = AFL_HTTPMETHOD_OPTIONS_S;
    m_methodStr[HttpPatch] = AFL_HTTPMETHOD_PATCH_S;

    /* 1xx 信息 */
    m_codeDesc[HttpStatusContinue] = AFL_HTTP_STATUS_CONTINUE_S;
    m_codeDesc[HttpStatusSwichingProtocols] = AFL_HTTP_STATUS_SWITCHING_PROTOCOLS_S;

    /* 2xx 成功 */
    m_codeDesc[HttpStatusOk] = AFL_HTTP_STATUS_OK_S;
    m_codeDesc[HttpStatsuCreated] = AFL_HTTP_STATUS_CREATED_S;
    m_codeDesc[HttpStatusAccepted] = AFL_HTTP_STATUS_ACCEPTED_S;
    m_codeDesc[HttpStatusNonAuthorizedInformation] =
        AFL_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION_S;
    m_codeDesc[HttpStatusNoContent] = AFL_HTTP_STATUS_NO_CONTENT_S;
    m_codeDesc[HttpStatusResetContent] = AFL_HTTP_STATUS_RESET_CONTENT_S;
    m_codeDesc[HttpStatusPartialContent] = AFL_HTTP_STATUS_PARTIAL_CONTENT_S;

    /* 3xx 重定向 */
    m_codeDesc[HttpStatusMultipleChoices] = AFL_HTTP_STATUS_MULTIPLE_CHOICES_S;
    m_codeDesc[HttpStatusMovedPermanetly] = AFL_HTTP_STATUS_MOVED_PERMANENTLY_S;
    m_codeDesc[HttpStatusFound] = AFL_HTTP_STATUS_FOUND_S;
    m_codeDesc[HttpStatusSeeOther] = AFL_HTTP_STATUS_SEE_OTHER_S;
    m_codeDesc[HttpStatusNotModified] = AFL_HTTP_STATUS_NOT_MODIFIED_S;
    m_codeDesc[HttpStatusUseProxy] = AFL_HTTP_STATUS_USE_PROXY_S;
    m_codeDesc[HttpStatusTemporaryRedirection] = AFL_HTTP_STATUS_TEMPORARY_REDIRECT_S;

    /* 4xx 客户端错误 */
    m_codeDesc[HttpStatusBadRequest] = AFL_HTTP_STATUS_BAD_REQUEST_S;
    m_codeDesc[HttpStatusUnauthorized] = AFL_HTTP_STATUS_UNAUTHORIZED_S;
    m_codeDesc[HttpStatusPaymentRequired] = AFL_HTTP_STATUS_PAYMENT_REQUIRED_S;
    m_codeDesc[HttpStatusForbidden] = AFL_HTTP_STATUS_FORBIDDEN_S;
    m_codeDesc[HttpStatusNotFound] = AFL_HTTP_STATUS_NOT_FOUND_S;
    m_codeDesc[HttpStatusMethodNotAllowed] = AFL_HTTP_STATUS_METHOD_NOT_ALLOWED_S;
    m_codeDesc[HttpStatusNotAcceptable] = AFL_HTTP_STATUS_NOT_ACCEPTABLE_S;
    m_codeDesc[HttpStatusProxyAuthenticationRequired] =
        AFL_HTTP_STATUS_PROXY_AUTHENICATION_REQUIRED_S;
    m_codeDesc[HttpStatusRequestTimeOut] = AFL_HTTP_STATUS_REQUEST_TIME_OUT_S;
    m_codeDesc[HttpStatusConflict] = AFL_HTTP_STATUS_CONFLICT_S;
    m_codeDesc[HttpStatusGone] = AFL_HTTP_STATUS_GONE_S;
    m_codeDesc[HttpStatusLengthRequired] = AFL_HTTP_STATUS_LENGTH_REQUIRED_S;
    m_codeDesc[HttpStatusProconditionFailed] = AFL_HTTP_STATUS_PRECONDITION_FAILED_S;
    m_codeDesc[HttpStatusRequestEntityTooLarge] = AFL_HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE_S;
    m_codeDesc[HttpStatusRequestURITooLarge] = AFL_HTTP_STATUS_REQUEST_URI_TOO_LARGE_S;
    m_codeDesc[HttpStatusUnsupportedMediaType] = AFL_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE_S;
    m_codeDesc[HttpStatusRequestedRangeNotSatisfiable] =
        AFL_HTTP_STATUS_REQUEST_RANGE_NOT_SATISFIABLE_S;
    m_codeDesc[HttpStatusExpectationFailed] = AFL_HTTP_STATUS_EXPECTATION_FAILED_S;

    /* 5xx 服务端错误 */
    m_codeDesc[HttpStatusInternalServerError] = AFL_HTTP_STATUS_INTERNAL_SERVER_ERROR_S;
    m_codeDesc[HttpStatusNotImplemented] = AFL_HTTP_STATUS_NOT_IMPLEMENTED_S;
    m_codeDesc[HttpStatusBadGateway] = AFL_HTTP_STATUS_BAD_GATEWAY_S;
    m_codeDesc[HttpStatusServiceUnavaliable] = AFL_HTTP_STATUS_SERVICE_UNAVAILABLE_S;
    m_codeDesc[HttpStatusGatewayTimeOut] = AFL_HTTP_STATUS_GATEWAY_TIME_OUT_S;
    m_codeDesc[HttpStatusHttpVersionNotSupported] = AFL_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED_S;
}

} // namespace net
} // namespace afl
