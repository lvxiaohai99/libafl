/**
 * @file   Log.cpp
 * @brief  全局日志门面实现（见 Log.h）。
 * @author libafl
 * @date   2026-09
 *
 * 保证：未显式 setup 时自动回落控制台；spdlog 抛异常时回落 fprintf，不拖垮启动。
 */
#include "afl/log/Log.h"

#include "afl/file/FileUtil.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <map>
#include <mutex>
#include <vector>

namespace afl
{
namespace log
{
namespace
{
/// 默认 logger 全局持有器（进程生命周期）
spdlog::logger* g_DefaultLogger = NULL;
std::mutex g_DefaultLoggerMutex;

/// 单条日志最大长度
const size_t kMaxLogLength = 4096;

/** @brief 解析级别字符串；失败返回 false */
bool parseLevel(const std::string& level, spdlog::level::level_enum& out)
{
    static const std::map<std::string, spdlog::level::level_enum> levelMap = {
        {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},   {"fatal", spdlog::level::critical},
        {"off", spdlog::level::off},
    };
    auto it = levelMap.find(level);
    if (it == levelMap.end())
    {
        return false;
    }
    out = it->second;
    return true;
}

/** @brief 滚动参数合法性：大小>0 且个数>=1 */
bool validRotate(std::size_t maxFileSizeBytes, std::size_t maxFiles)
{
    return maxFileSizeBytes > 0 && maxFiles >= 1;
}

void applyPatternUnlocked(spdlog::logger& logger, const std::string& pattern)
{
    try
    {
        logger.set_pattern(pattern.empty() ? kDefaultLogPattern : pattern);
    }
    catch (...)
    {
        // pattern 异常不影响进程
    }
}

/** @brief 纯控制台紧急输出（不依赖 spdlog 成功） */
void fallbackFprintf(spdlog::level::level_enum level, const char* file, int line, const char* msg)
{
    const char* lv = "I";
    switch (level)
    {
    case spdlog::level::trace:
        lv = "T";
        break;
    case spdlog::level::debug:
        lv = "D";
        break;
    case spdlog::level::info:
        lv = "I";
        break;
    case spdlog::level::warn:
        lv = "W";
        break;
    case spdlog::level::err:
        lv = "E";
        break;
    case spdlog::level::critical:
        lv = "C";
        break;
    default:
        break;
    }
    const char* base = file ? file : "?";
    const char* slash = strrchr(base, '/');
    if (slash)
    {
        base = slash + 1;
    }
    std::fprintf(stderr, "[fallback|%s|afl|%s(%d)]: %s\n", lv, base, line, msg ? msg : "");
    std::fflush(stderr);
}

/** 惰性创建控制台默认 logger；失败返回 nullptr */
spdlog::logger* createDefaultLogger()
{
    static spdlog::logger* console = nullptr;
    static bool tried = false;
    if (console != nullptr)
    {
        return console;
    }
    if (tried)
    {
        return nullptr;
    }
    tried = true;
    try
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console = new spdlog::logger("afl", sink);
        console->set_level(spdlog::level::info);
        applyPatternUnlocked(*console, std::string());
    }
    catch (...)
    {
        console = nullptr;
        std::fprintf(stderr, "[fallback|C|afl|Log.cpp]: createDefaultLogger failed, use stderr\n");
        std::fflush(stderr);
    }
    return console;
}

bool installLogger(spdlog::logger* logger, spdlog::level::level_enum lv)
{
    if (!logger)
    {
        return false;
    }
    try
    {
        logger->set_level(lv);
        logger->flush_on(lv);
        applyPatternUnlocked(*logger, std::string());
        {
            std::lock_guard<std::mutex> guard(g_DefaultLoggerMutex);
            g_DefaultLogger = logger;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace

void applyLogPattern(spdlog::logger& logger, const std::string& pattern)
{
    applyPatternUnlocked(logger, pattern);
}

spdlog::logger& getDefaultLogger()
{
    std::lock_guard<std::mutex> guard(g_DefaultLoggerMutex);
    if (g_DefaultLogger == NULL)
    {
        g_DefaultLogger = createDefaultLogger();
    }
    if (g_DefaultLogger == NULL)
    {
        // 极端情况：返回永不析构的空操作 logger（stdout sink）
        try
        {
            static auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
            static spdlog::logger emergency("afl", sink);
            applyPatternUnlocked(emergency, std::string());
            g_DefaultLogger = &emergency;
        }
        catch (...)
        {
            // 再退一步：无 sink 的 logger 仍可构造，消息被丢弃但不崩
            static spdlog::logger blackhole("afl", spdlog::sinks_init_list{});
            g_DefaultLogger = &blackhole;
        }
    }
    return *g_DefaultLogger;
}

void setDefaultLogger(spdlog::logger& logger)
{
    std::lock_guard<std::mutex> guard(g_DefaultLoggerMutex);
    applyPatternUnlocked(logger, std::string());
    g_DefaultLogger = &logger;
}

void logPrintf(spdlog::level::level_enum level, const char* file, int line, const char* fmt, ...)
{
    char buffer[kMaxLogLength];
    buffer[0] = '\0';

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt ? fmt : "", args);
    va_end(args);

    if (n < 0)
    {
        return;
    }

    try
    {
        spdlog::logger* logger = nullptr;
        {
            std::lock_guard<std::mutex> guard(g_DefaultLoggerMutex);
            if (g_DefaultLogger == NULL)
            {
                g_DefaultLogger = createDefaultLogger();
            }
            logger = g_DefaultLogger;
        }
        if (logger == nullptr)
        {
            fallbackFprintf(level, file, line, buffer);
            return;
        }
        if (logger->should_log(level))
        {
            logger->log(spdlog::source_loc{file, line, ""}, level,
                        spdlog::string_view_t(buffer, static_cast<size_t>(n) < sizeof(buffer)
                                                          ? static_cast<size_t>(n)
                                                          : sizeof(buffer) - 1));
        }
    }
    catch (...)
    {
        fallbackFprintf(level, file, line, buffer);
    }
}

bool setupConsoleLogger(const std::string& level, const std::string& loggerName)
{
    spdlog::level::level_enum lv;
    if (!parseLevel(level, lv) || loggerName.empty())
    {
        return false;
    }

    try
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        spdlog::logger* logger = new spdlog::logger(loggerName, sink);
        return installLogger(logger, lv);
    }
    catch (...)
    {
        fallbackFprintf(spdlog::level::err, __FILE__, __LINE__, "setupConsoleLogger failed");
        return false;
    }
}

bool setupFileLogger(const std::string& dir, const std::string& baseName, const std::string& level,
                     std::size_t maxFileSizeBytes, std::size_t maxFiles,
                     const std::string& loggerName)
{
    spdlog::level::level_enum lv;
    if (!parseLevel(level, lv) || !validRotate(maxFileSizeBytes, maxFiles) || loggerName.empty())
    {
        return false;
    }

    try
    {
        if (!afl::file::FileUtil::createRecursionDir(dir.c_str()))
        {
            // 文件目录失败 → 回落控制台，进程仍可启动
            fallbackFprintf(spdlog::level::warn, __FILE__, __LINE__,
                            "setupFileLogger: mkdir failed, fallback console");
            return setupConsoleLogger(level, loggerName);
        }

        const std::string file = dir + "/" + baseName;
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, maxFileSizeBytes,
                                                                          maxFiles);
        spdlog::logger* logger = new spdlog::logger(loggerName, sink);
        return installLogger(logger, lv);
    }
    catch (...)
    {
        fallbackFprintf(spdlog::level::warn, __FILE__, __LINE__,
                        "setupFileLogger: exception, fallback console");
        return setupConsoleLogger(level, loggerName);
    }
}

bool setupAppLogger(const std::string& dir, const std::string& baseName,
                    const std::string& fileLevel, const std::string& consoleLevel,
                    std::size_t maxFileSizeBytes, std::size_t maxFiles,
                    const std::string& loggerName)
{
    spdlog::level::level_enum fileLv;
    spdlog::level::level_enum consoleLv;
    if (!parseLevel(fileLevel, fileLv) || !parseLevel(consoleLevel, consoleLv) ||
        !validRotate(maxFileSizeBytes, maxFiles) || loggerName.empty())
    {
        return false;
    }

    try
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(consoleLv);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(consoleSink);

        bool fileOk = false;
        if (afl::file::FileUtil::createRecursionDir(dir.c_str()))
        {
            try
            {
                auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    dir + "/" + baseName, maxFileSizeBytes, maxFiles);
                fileSink->set_level(fileLv);
                sinks.push_back(fileSink);
                fileOk = true;
            }
            catch (...)
            {
                fallbackFprintf(spdlog::level::warn, __FILE__, __LINE__,
                                "setupAppLogger: file sink failed, console only");
            }
        }
        else
        {
            fallbackFprintf(spdlog::level::warn, __FILE__, __LINE__,
                            "setupAppLogger: mkdir failed, console only");
        }

        auto* logger = new spdlog::logger(loggerName, sinks.begin(), sinks.end());
        const spdlog::level::level_enum loggerLv = (fileLv < consoleLv) ? fileLv : consoleLv;
        if (!installLogger(logger, loggerLv))
        {
            return setupConsoleLogger(consoleLevel, loggerName);
        }
        if (!fileOk)
        {
            // 已装控制台，仍算成功
        }
        return true;
    }
    catch (...)
    {
        fallbackFprintf(spdlog::level::warn, __FILE__, __LINE__,
                        "setupAppLogger: exception, fallback console");
        return setupConsoleLogger(consoleLevel, loggerName);
    }
}

} // namespace log
} // namespace afl
