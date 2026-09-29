/**
 * @file   Timer.h
 * @brief  定时器封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include "afl/net/CallBacks.h"
namespace afl
{
namespace net
{
class EventLoop;

class Timer
{
public:
    Timer(TimerId id, const TimerCallback& cb, const TimeStamp& when, double interval)
        : m_id(id), m_callback(cb), m_when(when), m_interval(interval)
    {
    }

    TimerId id() const { return m_id; }

    TimeStamp expires_at() const { return m_when; }

    bool repeat() const { return m_interval > 0; }

    void trigger() const { m_callback(); }

    void restart(const TimeStamp& now)
    {
        if (repeat())
        {
            m_when = now + m_interval;
        }
        else
        {
            m_when = TimeStamp::invalid();
        }
    }

private:
    TimerId m_id;
    TimerCallback m_callback;
    TimeStamp m_when;
    double m_interval; // second
};

inline bool operator<(const Timer& lhs, const Timer& rhs)
{
    return lhs.expires_at() < rhs.expires_at();
}

} // namespace net
} // namespace afl
