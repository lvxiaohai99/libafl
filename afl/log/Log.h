/**
 * @file   Log.h
 * @brief  全局日志门面：printf 风格 m_LOG* 宏，底层对接 spdlog。
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "spdlog/spdlog.h"

#include <cstddef>
#include <string>
#include <vector>

namespace afl
{
namespace log
{
/** @brief 滚动日志默认：单文件 10MB */
const std::size_t kDefaultMaxFileSizeBytes = 10 * 1024 * 1024;
/** @brief 滚动日志默认：最多保留 6 个文件（含当前） */
const std::size_t kDefaultMaxFiles = 6;

/**
 * @brief 默认日志行格式
 *
 * 示例：
 * `[2021-01-03 17:11:41.843|I|mt_data|config_client.cpp(232)]: message`
 */
const char* const kDefaultLogPattern = "[%Y-%m-%d %H:%M:%S.%e|%^%L%$|%n|%s(%#)]: %v";

/**
 * @brief 给 logger 套上 AFL 统一日志格式（setup* / LoggerManager 会自动调用）
 * @param logger 目标 logger
 * @param pattern spdlog pattern；空则用 kDefaultLogPattern
 */
void applyLogPattern(spdlog::logger& logger, const std::string& pattern = std::string());

/**
 * @brief printf 风格日志输出（内部使用，建议直接使用 m_LOG* 宏）
 * @param level  spdlog 日志级别
 * @param file   源文件名（__FILE__）
 * @param line   源文件行号（__LINE__）
 * @param fmt    printf 格式串
 */
void logPrintf(spdlog::level::level_enum level, const char* file, int line, const char* fmt, ...)
    __attribute__((format(printf, 4, 5)));

/** @brief 获取默认 logger（未 setup 时自动回落控制台；失败亦不抛异常） */
spdlog::logger& getDefaultLogger();

/**
 * @brief 替换默认 logger
 * @param logger 新的默认 logger（调用方保证生命周期）
 */
void setDefaultLogger(spdlog::logger& logger);

/**
 * @brief 便捷初始化：滚动文件日志
 * @param dir              日志目录（自动创建）
 * @param baseName         日志文件名，如 "default.log"
 * @param level            级别字符串："trace/debug/info/warn/error/fatal"
 * @param maxFileSizeBytes 单文件上限（字节），默认 10MB
 * @param maxFiles         滚动保留个数（含当前文件），默认 6
 * @param loggerName       logger 名（出现在格式的第三段），默认 "afl"
 * @return 成功返回 true
 */
bool setupFileLogger(const std::string& dir, const std::string& baseName,
                     const std::string& level = "info",
                     std::size_t maxFileSizeBytes = kDefaultMaxFileSizeBytes,
                     std::size_t maxFiles = kDefaultMaxFiles,
                     const std::string& loggerName = "afl");

/**
 * @brief 便捷初始化：控制台日志
 * @param level      级别字符串
 * @param loggerName logger 名，默认 "afl"
 * @return 成功返回 true
 */
bool setupConsoleLogger(const std::string& level = "info", const std::string& loggerName = "afl");

/**
 * @brief 便捷初始化：控制台 + 滚动文件（推荐应用入口使用）
 * @note 文件侧失败时自动回落「仅控制台」，一般仍返回 true，不拖垮启动
 * @param dir              日志目录（自动创建）
 * @param baseName         日志文件名，如 "app.log"
 * @param fileLevel        文件侧级别，默认 "info"
 * @param consoleLevel     控制台级别，默认 "warn"（少刷屏；想看全量可传 "info"）
 * @param maxFileSizeBytes 单文件上限（字节），默认 10MB
 * @param maxFiles         滚动保留个数（含当前文件），默认 6
 * @param loggerName       logger 名（如 "mt_data"），默认 "afl"
 * @return 成功返回 true（含回落控制台成功）；参数非法返回 false
 */
bool setupAppLogger(const std::string& dir, const std::string& baseName,
                    const std::string& fileLevel = "info",
                    const std::string& consoleLevel = "warn",
                    std::size_t maxFileSizeBytes = kDefaultMaxFileSizeBytes,
                    std::size_t maxFiles = kDefaultMaxFiles,
                    const std::string& loggerName = "afl");

} // namespace log
} // namespace afl

/** @name printf 风格日志宏（定义见 LogMacros.h，幂等可重复包含） */
///@{
#include "afl/log/LogMacros.h"
///@}

/**
 * @brief 十六进制 dump 日志（INFO 级别输出）
 * @param ptr 数据指针
 * @param len 数据长度
 */
#define gDump(ptr, len)                                                                            \
    do                                                                                             \
    {                                                                                              \
        const char* cptr = (const char*)(ptr);                                                     \
        if (cptr)                                                                                  \
        {                                                                                          \
            std::stringstream ss;                                                                  \
            for (decltype(len) i = 0; i < (len); i++)                                              \
            {                                                                                      \
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)cptr[i] \
                   << " ";                                                                         \
            }                                                                                      \
            LOG_INFO("%s", ss.str().c_str());                                                      \
        }                                                                                          \
    } while (0)

#include <iomanip>
#include <sstream>
