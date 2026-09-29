/**
 * @file   TimerQueue.cpp
 * @brief  定时器队列的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/TimerQueue.h"
#include "afl/net/Timer.h"
#include "afl/net/EventLoop.h"
#include "afl/log/Log.h"
#include <limits.h>
#include "afl/time/StopWatch.h"
namespace afl
{
namespace net
{
TimerQueue::TimerQueue(EventLoop* loop) : m_loop(loop), m_callingTimesFunctor(false) {}

TimerQueue::~TimerQueue()
{
    for (TimerList::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
    {
        delete it->second;
    }
    m_timers.clear();
    m_activeTimers.clear();
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, const TimeStamp& when, double interval)
{
    TimerId id = ++m_atomic;
    Timer* timer = new Timer(id, cb, when, interval);
    m_loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return id;
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    m_loop->assertInLoopThread();
    addTimer(timer);
}

void TimerQueue::addTimer(Timer* timer)
{
    m_timers.insert(std::make_pair(timer->expires_at(), timer));
    m_activeTimers.insert(std::make_pair(timer->id(), timer));
}

void TimerQueue::cancelTimer(TimerId id)
{
    m_loop->runInLoop(std::bind(&TimerQueue::cancelTimerInLoop, this, id));
}

void TimerQueue::cancelTimerInLoop(TimerId id)
{
    m_loop->assertInLoopThread();
    TimerMap::iterator it = m_activeTimers.find(id);
    if (it != m_activeTimers.end())
    {
        TimerList::iterator iter =
            m_timers.find(std::make_pair(it->second->expires_at(), it->second));
        assert(iter != m_timers.end());
        Timer* timer = it->second;
        delete timer;
        m_activeTimers.erase(it);
        m_timers.erase(iter);
    }
    else // not find，maybe non exist or calling functor
    {
        if (m_callingTimesFunctor) // exist, but calling functor
        {
            //assert(m_activeTimers.find(id) != m_activeTimers.end());
            //Timer *timer = m_activeTimers[id];
            //LOG_INFO("timer id [%d][%d]", timer->id(), id);
            //assert(timer->id() == id);
            //m_cancelTimers.insert(std::make_pair(id, timer));
            m_cancelTimers.insert(id);
        }
    }

    assert(m_timers.size() == m_activeTimers.size());
}

TimeStamp TimerQueue::getNearestExpiration() const
{
    if (m_timers.empty())
        return TimeStamp::invalid();
    return m_timers.begin()->first;
}

void TimerQueue::runTimer(const TimeStamp& now)
{
    if (m_timers.empty())
        return;

    m_callingTimesFunctor = true;
    m_cancelTimers.clear(); // just for saving cancel timers when runTimer

    std::vector<Entry> expired = getExpiredTimers(now);

    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        it->second->trigger();
    }

    m_callingTimesFunctor = false;

    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        Timer* timer = it->second;
        if (timer->repeat() && m_cancelTimers.find(timer->id()) == m_cancelTimers.end())
        {
            timer->restart(now);
            addTimer(timer);
        }
        else // timer just run once or has already being canceled
        {
            delete timer;
        }
    }

    expired.clear();
    assert(m_timers.size() == m_activeTimers.size());
}

std::vector<TimerQueue::Entry> TimerQueue::getExpiredTimers(const TimeStamp& now)
{
    std::vector<Entry> expired;

    Entry piovt(now, reinterpret_cast<Timer*>(INT_MAX));
    TimerList::iterator end =
        m_timers.lower_bound(piovt); // return the pos of that not less than piovt
    assert(end == m_timers.end() || now == end->first || now < end->first);
    std::copy(m_timers.begin(), end, std::back_inserter(expired));

    m_timers.erase(m_timers.begin(), end);
    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        m_activeTimers.erase(it->second->id());
    }

    LOG_DEBUG("TimerQueue::getExpiredTimers [%d][%d][%d]", expired.size(), m_timers.size(),
             m_activeTimers.size());
    return expired;
}

} // namespace net
} // namespace afl
