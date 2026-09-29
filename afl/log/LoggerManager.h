/**
 * @file   LoggerManager.h
 * @brief  多 Logger / 多 Sink 管理（基于 spdlog）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "spdlog/spdlog.h"
#include "afl/file/FileUtil.h"
#include "afl/log/PathAnalyzer.h"

#include <sstream>
#include <unordered_set>

namespace afl
{
namespace log
{
/**
 * @brief 管理具名 logger 与 sink 路径解析
 *
 * sink 路径语法由 PathAnalyzer 解析，例如：
 * - `file:logs/app.log`
 * - `stdout:` / `stderr:`
 * - `tcp:127.0.0.1:514`
 */
class LoggerManager final
{
public:
    /**
     * @brief 构造
     * @param root  相对文件 sink 的根目录
     * @param level 默认日志级别字符串（debug/info/...）
     */
    explicit LoggerManager(std::string root = "./", std::string level = "debug");
    ~LoggerManager();

    /**
     * @brief 注册或获取具名 logger
     * @param name  logger 名
     * @param sinks sink 路径列表；空则使用默认 sink
     * @return logger 引用
     */
    spdlog::logger& registerLogger(std::string name, std::vector<std::string> sinks = {});

    /**
     * @brief 注销 logger
     * @param logger logger 引用
     */
    void unregisterLogger(spdlog::logger& logger);

    /**
     * @brief 注销 logger
     * @param logger logger 指针
     */
    void unregisterLogger(spdlog::logger* logger);

private:
    std::vector<spdlog::sink_ptr> makeSinkPtrs(std::vector<std::string>&& sinks);
    spdlog::sink_ptr makeSinkPtr(std::string sinks);

    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

    std::mutex m_lock;
    std::string m_defaultLoggerLevel;
    std::string m_rootDir;
    std::vector<spdlog::sink_ptr> m_defaultLogSinkPtrs;
    std::unordered_set<spdlog::logger*> m_loggers;
    std::unordered_map<std::string, spdlog::sink_ptr> m_sinkPtrTable;

    static std::unordered_map<std::string, spdlog::level::level_enum> k_logLevelMap;
};

#define DEEP_DEBUG
#define LOGGER_COMM_ALIAS_NAME __alias__logger__alias__

#ifdef DEEP_DEBUG
#define gLogT(...) LOGGER_COMM_ALIAS_NAME.trace(__VA_ARGS__)
#define gLogD(...) LOGGER_COMM_ALIAS_NAME.debug(__VA_ARGS__)
#define gLogI(...) LOGGER_COMM_ALIAS_NAME.info(__VA_ARGS__)
#define gLogW(...) LOGGER_COMM_ALIAS_NAME.warn(__VA_ARGS__)
#define gLogE(...) LOGGER_COMM_ALIAS_NAME.error(__VA_ARGS__)
#define gLogF(...) LOGGER_COMM_ALIAS_NAME.critical(__VA_ARGS__)
#else
#define gLogT(...)
#define gLogD(...)
#define gLogI(...)
#define gLogW(...)
#define gLogE(...)
#define gLogF(...)
#endif

} // namespace log
} // namespace afl

// 保证无论包含顺序如何，LOG_* 宏对使用方始终可用
#include "afl/log/Log.h"
