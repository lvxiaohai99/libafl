/**
 * @file   Condition.h
 * @brief  条件变量封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"

namespace afl
{
namespace concurrency
{
/** @brief 与 Mutex 配合的条件变量 */
class Condition : public afl::base::NonCopy
{
public:
    explicit Condition(Mutex& mu) : m_mutex(mu) { pthread_cond_init(&m_condition, NULL); }

    ~Condition() { pthread_cond_destroy(&m_condition); }

public:
    void wait() { (void)pthread_cond_wait(&m_condition, m_mutex.getMutex()); }

    /** @brief 限时等待；仅超时时返回 true */
    bool timedWait(int millisecond)
    {
        assert(millisecond >= 0);
        struct timespec abstime;
        (void)clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += millisecond / 1000;
        abstime.tv_nsec += (millisecond % 1000) * 1000000; // 1 us = 1000000 ns
        while (abstime.tv_nsec >= 1000000000L)
        {
            ++abstime.tv_sec;
            abstime.tv_nsec %= 1000000000L;
        }
        return ETIMEDOUT == pthread_cond_timedwait(&m_condition, m_mutex.getMutex(), &abstime);
    }

    void notifyOne() { pthread_cond_signal(&m_condition); }

    void notifyAll() { pthread_cond_broadcast(&m_condition); }

private:
    Mutex& m_mutex;
    pthread_cond_t m_condition;
};

} // namespace concurrency
} // namespace afl
