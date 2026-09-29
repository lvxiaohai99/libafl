/**
 * @file   ProcessTest.cpp
 * @brief  process 模块单元测试（非破坏性 API）
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>

#include "afl/process/ProcessUtil.h"

using namespace afl::process;

TEST(ProcessTest, PidAndName)
{
    EXPECT_GT(ProcessUtil::pid(), 0);
    EXPECT_FALSE(ProcessUtil::pidString().empty());
    EXPECT_FALSE(ProcessUtil::procname().empty());
}

TEST(ProcessTest, HostAndUser)
{
    EXPECT_FALSE(ProcessUtil::hostname().empty());
    // username 可能为空（容器环境），只要求不抛异常
    EXPECT_NO_THROW(ProcessUtil::username());
}

TEST(ProcessTest, TimingAndCpu)
{
    EXPECT_GT(ProcessUtil::startTime().microSeconds(), 0);
    EXPECT_GE(ProcessUtil::elapsedTime(), 0);
    ProcessUtil::CpuTime ct = ProcessUtil::cpuTime();
    EXPECT_GE(ct.userSeconds, 0.0);
    EXPECT_GE(ct.systemSeconds, 0.0);
}

TEST(ProcessTest, OpenFilesAndThreads)
{
    EXPECT_GE(ProcessUtil::openedFiles(), 0);
    EXPECT_GE(ProcessUtil::maxOpenFiles(), 0);
    EXPECT_GE(ProcessUtil::numThreads(), 1);
    std::vector<pid_t> tids = ProcessUtil::threads();
    EXPECT_FALSE(tids.empty());
}

TEST(ProcessTest, ExePathAndStat)
{
    EXPECT_FALSE(ProcessUtil::exePath().empty());
    EXPECT_FALSE(ProcessUtil::procStatus().empty());
    EXPECT_FALSE(ProcessUtil::procStat().empty());
}
