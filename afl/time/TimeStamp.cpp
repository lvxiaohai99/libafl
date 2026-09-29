/**
 * @file   TimeStamp.cpp
 * @brief  时间戳的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/time/TimeStamp.h"
#include "afl/base/Common.h"
#include <stdio.h>
#include <time.h>

#include <sys/time.h>
#define AFL_LOCALTIME(a, b) localtime_r(a, b)
#define AFL_GMTIME(a, b) gmtime_r(a, b)

namespace afl
{
namespace time
{
TimeStamp::TimeStamp() : m_microSeconds(0) {}

TimeStamp::TimeStamp(int64_t ms) : m_microSeconds(ms) {}

/*static*/ TimeStamp TimeStamp::invalid()
{
    return TimeStamp();
}

/*static*/ TimeStamp TimeStamp::now(bool isSystemTime /* = false*/)
{
    int64_t step;
    if (isSystemTime)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        step = ((int64_t)tv.tv_sec) * 1000 * 1000 + tv.tv_usec;
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        step = ((int64_t)ts.tv_sec) * 1000 * 1000 + ts.tv_nsec / 1000;
    }
    return TimeStamp(step);
}

struct tm TimeStamp::getTm(bool showlocaltime /* = true*/) const
{
    struct tm tm_time;
    time_t seconds = static_cast<time_t>(m_microSeconds / AFL_USEC_PER_SEC);
    if (showlocaltime)
        AFL_LOCALTIME(&seconds, &tm_time);
    else
        AFL_GMTIME(&seconds, &tm_time);
    return tm_time;
}

std::string TimeStamp::toString(bool showlocaltime /* = true*/) const
{
    struct tm tm_time;
    time_t seconds = static_cast<time_t>(m_microSeconds / AFL_USEC_PER_SEC);
    int microseconds = m_microSeconds % (AFL_USEC_PER_SEC);

    if (showlocaltime)
        AFL_LOCALTIME(&seconds, &tm_time);
    else
        AFL_GMTIME(&seconds, &tm_time);

    char buf[32] = {0};
    AFL_SNPRINTF(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d:%06d", tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min,
                 tm_time.tm_sec, microseconds);

    return buf;
}

} // namespace time
} // namespace afl
