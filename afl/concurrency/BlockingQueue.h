/**
 * @file   BlockingQueue.h
 * @brief  同步阻塞队列，可工作于多线程环境下，用于线程之间数据存取
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/concurrency/Mutex.h"
#include "afl/concurrency/Condition.h"
#include <queue>

namespace afl
{
namespace concurrency
{
/** @brief 先进先出 */
struct tagFIFO
{
};
/** @brief 先进后出 */
struct tagFILO
{
};
/** @brief 按优先级 */
struct tagPRIO
{
};

/** @brief 阻塞队列，支持 FIFO/FILO/优先级策略 */
template <typename Job, typename Queue = std::queue<Job>, typename Order = tagFIFO>
class BlockingQueue : public afl::base::NonCopy
{
public:
    typedef Job JobType;
    typedef Queue QueueType;
    typedef afl::concurrency::Mutex MutexType;
    typedef afl::concurrency::LockGuard<MutexType> LockGuard;
    typedef afl::concurrency::Condition ConditionType;

public:
    BlockingQueue() : m_stopFlag(false), m_mutex(), m_hasJob(m_mutex) {}

    ~BlockingQueue() { stop(); }

public:
    bool push(const JobType& job)
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag)
            return false;
        m_queue.push(job);
        m_hasJob.notifyOne();
        return true;
    }

    bool push(JobType&& job)
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag)
            return false;
        m_queue.push(std::move(job));
        m_hasJob.notifyOne();
        return true;
    }

    bool pop(JobType& job)
    {
        LockGuard lock(m_mutex);
        while (m_queue.empty() && !m_stopFlag)
        {
            m_hasJob.wait();
        }
        if (m_stopFlag)
        {
            return false;
        }
        return popOne(job, Order());
    }

    JobType pop()
    {
        LockGuard lock(m_mutex);
        while (m_queue.empty() && !m_stopFlag)
        {
            m_hasJob.wait();
        }
        if (m_stopFlag)
        {
            return JobType();
        }
        JobType job;
        popOne(job, Order());
        return job;
    }

    bool pop(std::vector<JobType>& vec, int pop_size = -1)
    {
        LockGuard lock(m_mutex);
        while (m_queue.empty() && !m_stopFlag)
        {
            m_hasJob.wait();
        }
        if (m_stopFlag)
        {
            return false;
        }

        if (pop_size <= 0)
            pop_size = m_queue.size();

        JobType job;
        while (pop_size-- > 0 && !m_stopFlag)
        {
            if (!popOne(job, Order()))
                break;
            else
                vec.push_back(job);
        }

        return true;
    }

    bool tryPop(JobType& job)
    {
        LockGuard lock(m_mutex);
        if (m_queue.empty() && !m_stopFlag)
            return false;
        return popOne(job, Order());
    }

    void stop()
    {
        {
            LockGuard lock(m_mutex);
            m_stopFlag = true;
        }
        m_hasJob.notifyAll();
    }

    size_t size() const
    {
        LockGuard lock(m_mutex);
        return m_queue.size();
    }

    bool empty()
    {
        LockGuard lock(m_mutex);
        return m_queue.empty();
    }

    template <typename Func>
    void foreach (const Func& func)
    {
        LockGuard lock(m_mutex);
        std::for_each(m_queue.begin(), m_queue.end(), func);
    }

private:
    template <typename T>
    bool popOne(JobType& job, T tag);

    //template <>
    bool popOne(JobType& job, tagFIFO /*tag*/)
    {
        if (m_queue.empty())
            return false;
        job = m_queue.front();
        m_queue.pop();
        return true;
    }

    //template <>
    bool popOne(JobType& job, tagFILO /*tag*/)
    {
        if (m_queue.empty())
            return false;
        job = m_queue.top();
        m_queue.pop();
        return true;
    }

    //template <>
    bool popOne(JobType& job, tagPRIO /*tag*/)
    {
        if (m_queue.empty())
            return false;
        job = m_queue.top();
        m_queue.pop();
        return true;
    }

protected:
    bool m_stopFlag;
    mutable MutexType m_mutex;
    ConditionType m_hasJob;
    QueueType m_queue;
};


#if defined(AFL_CXX11_ENABLED) || (_MSC_VER >= 1700) /// VS2010 不支持 using 别名

template <typename Job>
using BlockingFifoQueue = BlockingQueue<Job, std::queue<Job>, tagFIFO>;

template <typename Job>
using BlockingFiloQueue = BlockingQueue<Job, std::stack<Job>, tagFILO>;

template <typename Job>
using BlockingPrioQueue = BlockingQueue<Job, std::priority_queue<Job>, tagPRIO>;

#endif

} // namespace concurrency
} // namespace afl
