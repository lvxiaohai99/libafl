/**
 * @file   Event.h
 * @brief  同步原语，可用于多个线程在某一事件发生时通知其他（多个）线程
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include "afl/concurrency/Condition.h"

namespace afl
{
namespace concurrency
{
/** @brief 手动/自动重置事件（类似 Win32 Event） */
class Event : public afl::base::NonCopy
{
public:
    /** @param signal 初始是否已触发
     *  @param autoreset wait 成功后是否自动复位 */
    explicit Event(bool signal = false, bool autoreset = true)
        : m_mutex(), m_condition(m_mutex), m_signaled(signal), m_autoReset(autoreset)
    {
    }

    ~Event() {}

public:
    /** @brief 阻塞直至事件被 set */
    void wait()
    {
        LockGuard<Mutex> lock(m_mutex);
        while (!m_signaled)
        {
            m_condition.wait();
        }

        if (m_autoReset)
        {
            m_signaled = false;
        }
    }

    bool timedWait(int millisecond)
    {
        LockGuard<Mutex> lock(m_mutex);
        if (!m_signaled)
        {
            m_condition.timedWait(millisecond);
        }

        if (!m_signaled)
        {
            return false;
        }

        if (m_autoReset)
        {
            m_signaled = false;
        }
        return true;
    }

    bool tryWait() { return timedWait(0); }

    void set()
    {
        LockGuard<Mutex> lock(m_mutex);
        m_signaled = true;
        m_condition.notifyAll();
    }

    void reset()
    {
        LockGuard<Mutex> lock(m_mutex);
        m_signaled = false;
    }

    bool autoReset() const { return m_autoReset; }

private:
    afl::concurrency::Mutex m_mutex;
    afl::concurrency::Condition m_condition;
    bool m_signaled;
    const bool m_autoReset;
};

} // namespace concurrency
} // namespace afl
