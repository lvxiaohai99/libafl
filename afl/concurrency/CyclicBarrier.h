/**
 * @file   CyclicBarrier.h
 * @brief  回环栅栏，让一组线程等待至某个状态后再全部同时执行
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include "afl/concurrency/Condition.h"
#include <functional>

namespace afl
{
namespace concurrency
{
class CyclicBarrier : afl::base::NonCopy
{
public:
    typedef std::function<void()> CyclicBarrierCallBack;

public:
    explicit CyclicBarrier(int parties, CyclicBarrier::CyclicBarrierCallBack cb = NULL)
        : m_parties(parties), m_count(parties), m_mutex(), m_condition(m_mutex), m_cb(cb)
    {
    }

    void reset()
    {
        LockGuard<Mutex> lock(m_mutex);
        nextGeneration();
    }

    int wait() { return dowait(false, 0); }

    /** @brief 触发栅栏所需的线程数 */
    int getParties() const { return m_parties; }

    /** @brief 当前仍在栅栏处等待的线程数（调试用） */
    int getNumberWaiting()
    {
        LockGuard<Mutex> lock(m_mutex);
        return m_parties - m_count;
    }

private:
    void nextGeneration()
    {
        // 通知上一代全部到达
        m_condition.notifyAll();
        // 初始化下一代计数
        m_count = m_parties;
    }

    int dowait(bool timed, int millisecond)
    {
        LockGuard<Mutex> lock(m_mutex);
        int index = --m_count;
        if (index == 0) // 最后一个到达，触发栅栏
        {
            if (m_cb)
            {
                m_cb();
            }
            nextGeneration();
            return 0;
        }

        // 等待触发、中断或超时
        for (;;)
        {
            if (!timed)
                m_condition.wait();
            else if (millisecond > 0)
                m_condition.timedWait(millisecond);

            return index;
        }
        return 0;
    }

private:
    int m_parties; /// 参与线程总数
    int m_count;   /// 尚未到达的线程数（从 parties 递减至 0）
    mutable Mutex m_mutex;
    Condition m_condition;
    CyclicBarrierCallBack m_cb;
};

} // namespace concurrency
} // namespace afl
