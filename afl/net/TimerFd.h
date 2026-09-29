/**
 * @file   TimerFd.h
 * @brief  timerfd 封装；需 Linux 内核 ≥ 2.6.25
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include <sys/timerfd.h>

using afl::time::TimeStamp;
namespace afl
{
namespace net
{
typedef int Timerfd;

class TimerfdHandler
{
public:
    TimerfdHandler(int clockid = CLOCK_MONOTONIC, int flags = TFD_NONBLOCK | TFD_CLOEXEC);
    ~TimerfdHandler();

public:
    /// 返回定时器描述符
    Timerfd fd() { return m_timerfd; }

    /// 设置新的超时时间(绝对时间)以及定时器循环间(<=0 表示只定时一), 单位：微
    void resetTimerfd(TimeStamp expiration, int interval_us = 0);

    /// 设置新的超时时间(相对时间)以及定时器循环间(<=0 表示只定时一), 单位：微
    void resetTimerfd(uint64_t next_expire_us, int interval_us = 0);

    /// 获得当前有多少个定时器超
    uint64_t read(uint64_t* howmany);

    /// 停止定时器
    void stop();

private:
    Timerfd m_timerfd;
};

} // namespace net
} // namespace afl
