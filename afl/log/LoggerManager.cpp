/**
 * @file   LoggerManager.cpp
 * @brief  日志管理器实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/log/LoggerManager.h"
#include "afl/log/Log.h"
#include "afl/net/NetUtil.h"

namespace afl
{
namespace log
{
std::unordered_map<std::string, spdlog::level::level_enum> LoggerManager::k_logLevelMap = {
    {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
    {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
    {"error", spdlog::level::err},   {"fatal", spdlog::level::critical},
    {"off", spdlog::level::off},
};

LoggerManager& getGlobalLoggerManager()
{
    static LoggerManager g_macro_logger_manager("/tmp/ncs/glog/", "off");
    return g_macro_logger_manager;
}

LoggerManager::LoggerManager(std::string root, std::string level) : m_rootDir(root)
{
    assert(level == "trace" || level == "debug" || level == "info" || level == "warn" ||
           level == "error" || level == "fatal" || level == "off");

    if (m_rootDir.empty())
        m_rootDir = '/';

    if (m_rootDir.front() != '/')
    {
        char pwd[512];
        if (!getcwd(pwd, sizeof(pwd)))
        {
            perror("getcwd");
            m_rootDir = '/';
        }
        else
        {
            std::string absDir(pwd);
            m_rootDir = absDir.append("/").append(m_rootDir);
            //          fprintf(stderr, "absolute log directory %s\n", m_rootDir.c_str());
        }
    }

    PathAnalyzer::addSinkMaker(std::unique_ptr<SinkMaker>(new FileSinkMaker(m_rootDir)));
    PathAnalyzer::addSinkMaker(std::unique_ptr<SinkMaker>(new StdSinkMaker()));
    PathAnalyzer::addSinkMaker(std::unique_ptr<SinkMaker>(new InetSinkMaker()));
    PathAnalyzer::addSinkMaker(std::unique_ptr<SinkMaker>(new SysSinkMaker()));

    m_defaultLoggerLevel = level;
    m_defaultLogSinkPtrs = makeSinkPtrs({"file://default.log"});
}

LoggerManager::~LoggerManager()
{
    spdlog::drop_all();
}

spdlog::logger& LoggerManager::registerLogger(std::string name, std::vector<std::string> sinks)
{
    std::lock_guard<std::mutex> Guard(m_lock);

    auto sinkPtrs = makeSinkPtrs(std::forward<std::vector<std::string>>(sinks));

    auto logger = new spdlog::logger(name, sinkPtrs.begin(), sinkPtrs.end());
    m_loggers.insert(logger);

    auto level = k_logLevelMap[m_defaultLoggerLevel];
    logger->set_level(level);
    logger->flush_on(level);
    applyLogPattern(*logger);

    return *logger;
}

void LoggerManager::unregisterLogger(spdlog::logger* logger)
{
    std::lock_guard<std::mutex> Guard(m_lock);

    auto iter = m_loggers.find(logger);

    if (iter == m_loggers.end())
    {
        fprintf(stderr, "Unregister Logger : Could not find the logger in Loggers Table!");
        return;
    }

    m_loggers.erase(iter);
    delete logger;
}

void LoggerManager::unregisterLogger(spdlog::logger& logger)
{
    unregisterLogger(&logger);
}

spdlog::sink_ptr LoggerManager::makeSinkPtr(std::string sink)
{
    /* 解析 sink 路径 */
    PathAnalyzer pathAnalyzer(sink);

    /* 路径非法 */
    if (!pathAnalyzer)
    {
        fprintf(stderr, "Could not open sink directory %s!\n", sink.c_str());
        return nullptr;
    }

    /* 若 sink 已存在则复用 */
    auto unique = pathAnalyzer.getNormalizedPath();
    //  fprintf(stderr, "Sink directory Unique %s!\n", unique.c_str());

    auto spair = m_sinkPtrTable.find(unique);
    if (spair != m_sinkPtrTable.end())
    {
        //      fprintf(stderr, "Sink directory Unique %s! use the established sink!\n", unique.c_str());
        return spair->second;
    }

    /* 创建新 sink_ptr */
    auto sinkPtr = pathAnalyzer.getNewSinkPtr();
    if (sinkPtr)
    {
        m_sinkPtrTable[unique] = sinkPtr;
    }

    return sinkPtr;
}

std::vector<spdlog::sink_ptr> LoggerManager::makeSinkPtrs(std::vector<std::string>&& sinks)
{
    std::vector<spdlog::sink_ptr> retval;

    if (sinks.empty())
        return m_defaultLogSinkPtrs;

    for (const auto& sink : sinks)
    {
        auto sinkPtr = makeSinkPtr(sink);
        if (sinkPtr)
            retval.push_back(sinkPtr);
    }

    return retval;
}

} // namespace log
} // namespace afl
