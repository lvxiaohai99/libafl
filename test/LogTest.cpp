/**
 * @file   LogTest.cpp
 * @brief  log 模块单元测试：Log 门面 / LoggerManager
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <string>

#include "afl/log/Log.h"
#include "afl/log/LoggerManager.h"

using namespace afl::log;

namespace
{
std::string readAll(const std::string& path)
{
    std::ifstream ifs(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

} // namespace

TEST(LogTest, SetupFileLoggerAndWrite)
{
    const std::string dir = "/tmp/libafl_test_log_" + std::to_string(::getpid());
    const std::string base = "ut";
    const std::string file = dir + "/" + base;

    ::remove(file.c_str());

    ASSERT_TRUE(setupFileLogger(dir, base, "info"));

    LOG_INFO("libafl unit test message %d", 42);
    LOG_WARN("warn message");
    LOG_DEBUG("this debug line should be filtered"); // level=info 时不应写入
    getDefaultLogger().flush();

    const std::string content = readAll(file);
    // flush_on(info)：info 及以上级别自动落盘
    EXPECT_TRUE(content.find("libafl unit test message 42") != std::string::npos);
    EXPECT_TRUE(content.find("warn message") != std::string::npos);
    EXPECT_TRUE(content.find("filtered") == std::string::npos);
    // 格式：[时间|I|afl|LogTest.cpp(行)]: ...
    EXPECT_TRUE(content.find("|I|afl|") != std::string::npos);
    EXPECT_TRUE(content.find("|W|afl|") != std::string::npos);
    EXPECT_TRUE(content.find("LogTest.cpp(") != std::string::npos);

    ::remove(file.c_str());
}

TEST(LogTest, SetupAppLoggerConsoleAndFile)
{
    const std::string dir = "/tmp/libafl_test_applog_" + std::to_string(::getpid());
    const std::string base = "app.log";
    const std::string file = dir + "/" + base;
    ::remove(file.c_str());

    ASSERT_TRUE(setupAppLogger(dir, base, "info", "warn"));
    LOG_INFO("file-only info line");
    LOG_WARN("both sinks warn line");
    getDefaultLogger().flush();

    const std::string content = readAll(file);
    EXPECT_TRUE(content.find("file-only info line") != std::string::npos);
    EXPECT_TRUE(content.find("both sinks warn line") != std::string::npos);

    EXPECT_FALSE(setupAppLogger(dir, base, "bad-level", "warn"));
    EXPECT_FALSE(setupAppLogger(dir, base, "info", "warn", 0, 6));
    EXPECT_FALSE(setupAppLogger(dir, base, "info", "warn", 1024, 0));
    ASSERT_TRUE(setupAppLogger(dir, base, "info", "warn", 1024 * 1024, 3));
    ::remove(file.c_str());
}

TEST(LogTest, LogWithoutSetupFallsBackToConsole)
{
    // 未 setup 时 LOG_* 不得抛异常 / 不得崩
    EXPECT_NO_THROW(LOG_INFO("fallback console log before setup %d", 1));
    EXPECT_EQ("afl", std::string(getDefaultLogger().name()));
}

TEST(LogTest, SetupFileLoggerFallsBackWhenDirImpossible)
{
    // 不可写路径时回落控制台，仍返回 true
    EXPECT_TRUE(setupFileLogger("/proc/libafl_no_perm_log", "x.log", "info"));
    EXPECT_NO_THROW(LOG_WARN("after file fallback"));
}

TEST(LogTest, LoggerManagerRegister)
{
    LoggerManager mgr("./", "debug");
    spdlog::logger& logger = mgr.registerLogger("ut-logger");
    EXPECT_EQ("ut-logger", std::string(logger.name()));

    logger.info("hello from ut-logger"); // 仅冒烟：不崩溃即可
    mgr.unregisterLogger(logger);
}
