/**
 * @file   CountDownLatch.h
 * @brief  同步辅助类，允许一个或多个线程等待其他线程完成一组操作后再继续执行
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include "afl/concurrency/Condition.h"

namespace afl
{
namespace concurrency
{
/** @brief 倒计时门闩，计数归零后唤醒等待线程 */
class CountDownLatch : afl::base::NonCopy
{
public:
    explicit CountDownLatch(int count) : m_count(count), m_mutex(), m_condition(m_mutex) {}

    void wait()
    {
        LockGuard<Mutex> lock(m_mutex);
        while (m_count > 0)
        {
            m_condition.wait();
        }
    }

    void countDown()
    {
        LockGuard<Mutex> lock(m_mutex);
        --m_count;
        if (m_count == 0)
        {
            m_condition.notifyAll();
        }
    }

    int getCount() const
    {
        LockGuard<Mutex> lock(m_mutex);
        return m_count;
    }

private:
    int m_count;
    mutable Mutex m_mutex;
    Condition m_condition;
};

} // namespace concurrency
} // namespace afl
