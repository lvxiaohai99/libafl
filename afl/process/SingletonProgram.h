/**
 * @file   SingletonProgram.h
 * @brief  单实例程序（防止重复启动）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"

#include <functional>
#include <string>

namespace afl
{
namespace process
{
class SingletonProgram : private afl::base::NonCopy
{
    using WorkCB = std::function<int(pid_t pid)>;

public:
    explicit SingletonProgram(std::string pidFile, WorkCB master = nullptr, WorkCB others = nullptr,
                              bool daemon = false, int nochdir = 1, int noclose = 0);

    virtual ~SingletonProgram();

    int run();

private:
    bool alreadyRunning();

    std::string m_pidFile;
    WorkCB m_master;
    WorkCB m_others;
    bool m_inDaemon;
    pid_t m_pid;
    int m_nochdir;
    int m_noclose;
};

} // namespace process
} // namespace afl
