/**
 * @file   StopWatch.h
 * @brief  高精度计时器（秒表）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <sys/time.h>

namespace afl
{
namespace time
{
#define GET_TICK_COUNT(a, b) ((a.tv_sec - b.tv_sec) * 1000000 + (a.tv_usec - b.tv_usec))

class StopWatch
{
public:
    StopWatch() { start(); }

public:
    void reset() { getTimeOfDay(&start_time, NULL); }
    static struct timeval now()
    {
        struct timeval now;
        getTimeOfDay(&now, NULL);
        return now;
    }
    float elapsedTime()
    {
        struct timeval now;
        getTimeOfDay(&now, NULL);
        return float(GET_TICK_COUNT(now, start_time) / 1000000.0);
    }
    float elapsedTimeInMill()
    {
        struct timeval now;
        getTimeOfDay(&now, NULL);
        return float(GET_TICK_COUNT(now, start_time) / 1000.0);
    }
    int64_t elapsedTimeInMicro()
    {
        timeval now;
        getTimeOfDay(&now, NULL);
        return GET_TICK_COUNT(now, start_time);
    }
    float diffTime(const struct timeval& start)
    {
        struct timeval now;
        getTimeOfDay(&now, NULL);
        return float(GET_TICK_COUNT(now, start) / 1000000.0);
    }
    float diffTime(const struct timeval& start, const struct timeval& end)
    {
        return float(GET_TICK_COUNT(end, start) / 1000000.0);
    }

private:
    void start() { reset(); }
    static void getTimeOfDay(struct timeval* tv, void* tz) { gettimeofday(tv, NULL); }

private:
    struct timeval start_time;
};

} // namespace time
} // namespace afl
