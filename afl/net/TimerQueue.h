/**
 * @file   TimerQueue.h
 * @brief  定时器队列
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include "afl/concurrency/Atomic.h"
#include "afl/net/Timer.h"
#include "afl/net/CallBacks.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

using afl::time::TimeStamp;
namespace afl
{
namespace net
{
class Timer;
class EventLoop;

class TimerQueue
{
    typedef std::pair<TimeStamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::unordered_map<TimerId, Timer*> TimerMap;
    typedef std::unordered_set<TimerId> CancelTimerList;

public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

public:
    TimerId addTimer(const TimerCallback& cb, const TimeStamp& when, double interval);
    void cancelTimer(TimerId id);
    TimeStamp getNearestExpiration() const;
    void runTimer(const TimeStamp& now);

private:
    void addTimerInLoop(Timer* timer);
    void cancelTimerInLoop(TimerId id);
    void addTimer(Timer* timer);
    std::vector<Entry> getExpiredTimers(const TimeStamp& now);

private:
    TimerList m_timers;
    TimerMap m_activeTimers;
    CancelTimerList m_cancelTimers;

    EventLoop* m_loop;
    afl::concurrency::Atomic<int> m_atomic;
    afl::concurrency::Atomic<bool> m_callingTimesFunctor;
};

} // namespace net
} // namespace afl
