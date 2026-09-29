/**
 * @file   PathAnalyzer.cpp
 * @brief  日志 sink 路径解析实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/log/PathAnalyzer.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog/sinks/tcp_sink.h"

#include <functional>
#include <algorithm>
#include <cctype>

namespace afl
{
namespace log
{
std::unordered_map<std::string, std::unique_ptr<SinkMaker>> PathAnalyzer::m_sinkMakers;

PathAnalyzer::PathAnalyzer(std::string path)
{
    m_valid = false;

    static std::function<bool(char)> isVisible = [&](char c) {
        return !std::isspace(c) && std::isprint(c);
    };

    /* trim */
    auto lpos = std::find_if(path.begin(), path.end(), isVisible);
    auto rpos = std::find_if(path.rbegin(), path.rend(), isVisible);
    path = path.substr(lpos - path.begin(),
                       path.size() - (rpos - path.rbegin()) - (lpos - path.begin()));

    if (path.empty() || std::find_if_not(path.begin(), path.end(), isVisible) != path.end())
    {
        return;
    }

    size_t pos = path.find("://");

    if (pos == std::string::npos)
    {
        m_key = path;
        m_value.clear();
    }
    else
    {
        m_key = path.substr(0, pos);
        m_value = path.substr(pos + 3, path.size() - pos - 3);
    }

    std::transform(m_key.begin(), m_key.end(), m_key.begin(), ::tolower);
    if (m_sinkMakers.find(m_key) != m_sinkMakers.end())
    {
        m_valid = true;
    }
}

spdlog::sink_ptr PathAnalyzer::getNewSinkPtr()
{
    return m_valid ? m_sinkMakers[m_key]->path2Sink(*this) : nullptr;
}

std::string PathAnalyzer::getNormalizedPath()
{
    return m_valid ? m_sinkMakers[m_key]->getNormalizedPath(*this) : std::string();
}


void PathAnalyzer::addSinkMaker(std::unique_ptr<SinkMaker> sm)
{
    m_sinkMakers.insert(std::make_pair(sm->getKey(), std::move(sm)));
}

} // namespace log
} // namespace afl
