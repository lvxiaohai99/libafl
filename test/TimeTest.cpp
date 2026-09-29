/**
 * @file   TimeTest.cpp
 * @brief  time 模块单元测试：DateTime / TimeStamp / StopWatch
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <regex>
#include <string>
#include <thread>

#include "afl/time/DateTime.h"
#include "afl/time/TimeStamp.h"
#include "afl/time/StopWatch.h"

using namespace afl::time;

TEST(TimeTest, DateTimeCurrentFormat)
{
    std::string now = DateTime::currentDateTime();
    // YYYY-MM-DD HH:MM:SS
    std::regex pattern(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
    EXPECT_TRUE(std::regex_match(now, pattern)) << "now=" << now;

    std::string date = DateTime::currentDate();
    EXPECT_EQ(10u, date.size()); // YYYY-MM-DD

    std::string time = DateTime::currentTime();
    EXPECT_EQ(8u, time.size()); // HH:MM:SS
}

TEST(TimeTest, DateTimeStringParseRoundTrip)
{
    struct tm tmv = {};
    ASSERT_TRUE(DateTime::stringToDataTime("2026-09-01 12:30:45", &tmv));
    EXPECT_EQ(2026 - 1900, tmv.tm_year);
    EXPECT_EQ(9 - 1, tmv.tm_mon);
    EXPECT_EQ(1, tmv.tm_mday);
    EXPECT_EQ(12, tmv.tm_hour);
    EXPECT_EQ(30, tmv.tm_min);
    EXPECT_EQ(45, tmv.tm_sec);

    std::string formatted = DateTime::dateTimeToString(&tmv);
    EXPECT_EQ("2026-09-01 12:30:45", formatted);

    // 非法格式返回 false
    struct tm bad = {};
    EXPECT_FALSE(DateTime::stringToDataTime("2026/09/01", &bad));
}

TEST(TimeTest, DateTimeStringToTimeT)
{
    time_t t = 0;
    ASSERT_TRUE(DateTime::stringToDataTime("1970-01-01 00:00:00", &t));
    // 时区相关的 epoch 基准，仅断言解析成功且值在合理范围
    EXPECT_LE(t, 24 * 3600);
}

TEST(TimeTest, IsLeapYear)
{
    EXPECT_TRUE(DateTime::isLeapYear(2000));
    EXPECT_TRUE(DateTime::isLeapYear(2024));
    EXPECT_FALSE(DateTime::isLeapYear(1900));
    EXPECT_FALSE(DateTime::isLeapYear(2023));
}

TEST(TimeTest, TimeStampNowAndToString)
{
    TimeStamp ts = TimeStamp::now();
    EXPECT_TRUE(ts.valid());

    TimeStamp inv = TimeStamp::invalid();
    EXPECT_FALSE(inv.valid());

    std::string s = ts.toString();
    EXPECT_FALSE(s.empty());

    TimeStamp ts2 = TimeStamp::now();
    EXPECT_TRUE(ts < ts2 || ts == ts2);
}

TEST(TimeTest, StopWatchMonotonic)
{
    StopWatch watch; // 构造时自动开始计时
    EXPECT_GE(watch.elapsedTimeInMicro(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int64_t us1 = watch.elapsedTimeInMicro();
    EXPECT_GE(us1, 40000); // 约 50ms（宽松下界）

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int64_t us2 = watch.elapsedTimeInMicro();
    EXPECT_GT(us2, us1); // 单调递增

    watch.reset();
    EXPECT_LE(watch.elapsedTimeInMicro(), us2);
}
