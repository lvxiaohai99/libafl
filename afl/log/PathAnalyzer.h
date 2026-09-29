/**
 * @file   PathAnalyzer.h
 * @brief  日志 sink 路径解析与各类 SinkMaker
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog/sinks/tcp_sink.h"

// syslog.h 定义的优先级宏（LOG_INFO/LOG_DEBUG/LOG_NOTICE/LOG_WARNING/LOG_ERR 等）
// 会与 afl/log/Log.h 的同名日志宏冲突：这里重新包含 LogMacros.h（幂等），
// 保证包含本头文件后 m_LOG* 始终指向 libafl 的日志门面。
// 如需使用 syslog(3) 优先级常量，请自行使用数字。
#include "afl/log/LogMacros.h"

#include "afl/file/FileUtil.h"
#include "afl/net/NetUtil.h"
#include "afl/base/Common.h"

#include <list>
#include <sstream>
#include <string>
#include <unordered_map>

namespace afl
{
namespace log
{
class PathAnalyzer;

/**
 * @brief 将解析后的 sink 路径构造成 spdlog sink
 */
class SinkMaker
{
public:
    virtual ~SinkMaker() {}

    virtual std::string getKey() = 0;
    virtual spdlog::sink_ptr path2Sink(const PathAnalyzer& result) = 0;
    virtual std::string getNormalizedPath(const PathAnalyzer& result) = 0;
};

/**
 * @brief 解析日志 sink URI（如 file://、stdout、inet://、syslog）
 */
class PathAnalyzer final
{
public:
    explicit PathAnalyzer(std::string path);

    spdlog::sink_ptr getNewSinkPtr();
    std::string getNormalizedPath();

    operator bool() { return m_valid; }

    const std::string& getKey() const { return m_key; }

    const std::string& getValue() const { return m_value; }

    static void addSinkMaker(std::unique_ptr<SinkMaker> sm);

private:
    bool m_valid;
    std::string m_key;
    std::string m_value;

    static std::unordered_map<std::string, std::unique_ptr<SinkMaker>> m_sinkMakers;
};


class StdSinkMaker : public SinkMaker
{
    virtual std::string getKey() { return "stdout"; }
    virtual spdlog::sink_ptr path2Sink(const PathAnalyzer& result)
    {
        return std::make_shared<spdlog::sinks::stdout_sink_mt>();
    }

    virtual std::string getNormalizedPath(const PathAnalyzer& result) { return "stdout"; }
};

class InetSinkMaker : public SinkMaker
{
    virtual std::string getKey() { return "inet"; }
    virtual spdlog::sink_ptr path2Sink(const PathAnalyzer& result)
    {
        auto pos = result.getValue().find_first_of(':');
        if (pos == std::string::npos)
        {
            std::stringstream ss;
            ss << "inet log sink directory " << result.getValue() << " illegal!" << std::endl;
            throw std::logic_error(ss.str());
        }

        auto ip = result.getValue().substr(0, pos);
        auto sport = result.getValue().substr(pos + 1, result.getValue().size() - pos);

        int port = std::atol(sport.c_str());
        if (!afl::net::NetUtil::isValidIpv4(ip.c_str()) || port > 65535 || port < 0)
        {
            std::stringstream ss;
            ss << "inet log sink directory " << result.getValue() << " illegal!" << std::endl;
            throw std::logic_error(ss.str());
        }

        spdlog::sinks::tcp_sink_config cfg(ip, port);
        return std::make_shared<spdlog::sinks::tcp_sink_mt>(cfg);
    }

    virtual std::string getNormalizedPath(const PathAnalyzer& result)
    {
        auto pos = result.getValue().find_first_of(':');
        if (pos == std::string::npos)
        {
            std::stringstream ss;
            ss << "inet log sink directory " << result.getValue() << " illegal!" << std::endl;
            throw std::logic_error(ss.str());
        }

        auto ip = result.getValue().substr(0, pos);
        auto sport = result.getValue().substr(pos + 1, result.getValue().size() - pos);

        int port = std::atol(sport.c_str());
        if (!afl::net::NetUtil::isValidIpv4(ip.c_str()) || port > 65535 || port < 0)
        {
            std::stringstream ss;
            ss << "inet log sink directory " << result.getValue() << " illegal!" << std::endl;
            throw std::logic_error(ss.str());
        }

        std::stringstream ss;
        ss << "inet://" << ip << ":" << port;

        return ss.str();
    }
};

class SysSinkMaker : public SinkMaker
{
    virtual std::string getKey() { return "syslog"; }
    virtual spdlog::sink_ptr path2Sink(const PathAnalyzer& result)
    {
        return std::make_shared<spdlog::sinks::syslog_sink_mt>("afl", 0, LOG_USER, false);
    }

    virtual std::string getNormalizedPath(const PathAnalyzer& result) { return "syslog"; }
};

class FileSinkMaker : public SinkMaker
{
public:
    FileSinkMaker(std::string dir, int size = 10 * 1024 * 1024, int num = 6)
        : m_defaultDir(dir), m_size(size), m_num(num)
    {
        assert(dir[0] == '/');
    }

    virtual std::string getKey() { return "file"; }
    virtual spdlog::sink_ptr path2Sink(const PathAnalyzer& result)
    {
        std::string file = m_defaultDir + "/" + result.getValue();
        if (!afl::file::FileUtil::createRecursionDir(
                afl::file::FileUtil::dirName(file.c_str()).c_str()))
        {
            std::stringstream ss;
            ss << "log directory " << afl::file::FileUtil::dirName(file.c_str()) << " illegal! "
               << std::endl;
            throw std::logic_error(ss.str());
        }

        return std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, m_size, m_num);
    }

    virtual std::string getNormalizedPath(const PathAnalyzer& result)
    {
        std::string file = m_defaultDir + "/" + result.getValue();
        if (!afl::file::FileUtil::createRecursionDir(
                afl::file::FileUtil::dirName(file.c_str()).c_str()))
        {
            std::stringstream ss;
            ss << "log directory " << afl::file::FileUtil::dirName(file.c_str()) << " illegal! "
               << std::endl;
            throw std::logic_error(ss.str());
        }

        std::list<std::string> pathSegments;

        std::string::size_type pos = file.find('/');
        std::string::size_type last_pos = 0;

        //        fprintf(stderr, "F %s\n", file.c_str());

        while (pos != std::string::npos)
        {
            last_pos = pos;
            pos = file.find('/', pos + 1);

            //          fprintf(stderr, " N %d  %d\n", last_pos, pos);

            std::string cur = pos != std::string::npos
                                  ? file.substr(last_pos + 1, pos - last_pos - 1)
                                  : file.substr(last_pos + 1, file.length() - last_pos - 1);

            if (cur == "..")
            {
                pathSegments.pop_back();
            }

            if (!cur.empty() && cur != ".")
            {
                pathSegments.push_back(cur);
            }
        }

        std::string retval = "file://";

        for (auto& s : pathSegments)
        {
            //          fprintf(stderr, " S %s\n", s.c_str());
            retval.append("/").append(s);
        }

        return retval;
    }

private:
    std::string m_defaultDir;
    int m_size;
    int m_num;
};


} // namespace log
} // namespace afl
