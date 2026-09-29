/**
 * @file   ConcurrentQueue.h
 * @brief  参考 java.util.concurrent 与 concurrent_queue 实现的简单并发队列
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/concurrency/Mutex.h"
#include <vector>
#include <queue>
#include <stack>


namespace afl
{
namespace concurrency
{
namespace ConcurrentQueueTraits
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
} // namespace ConcurrentQueueTraits

/** @brief 带容量上限的并发队列（非阻塞 push/pop） */
template <typename T, typename Queue = std::queue<T>,
          typename Order = ConcurrentQueueTraits::tagFIFO>
struct ConcurrentQueue
{
public:
    typedef ConcurrentQueue<T, Queue, Order> this_type;
    typedef Queue QueueType;
    typedef afl::concurrency::Mutex MutexType;
    typedef afl::concurrency::LockGuard<MutexType> LockGuard;

    explicit ConcurrentQueue(size_t capacity = size_t(0))
        : m_stopFlag(false), m_capacity(capacity), m_mutex()
    {
    }

    inline bool push(const T& v)
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag)
            return false;
        if (m_capacity > 0 && m_queue.size() >= m_capacity)
            return false;
        m_queue.push(v);
        return true;
    }

    inline bool push(T&& v)
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag)
            return false;
        if (m_capacity > 0 && m_queue.size() >= m_capacity)
            return false;
        m_queue.push(std::move(v));
        return true;
    }

    /** @brief 兼容 std::back_inserter 的 pushBack */
    inline void pushBack(const T& data) { push(data); }

    inline void pushBack(T&& data) { push(std::move(data)); }

    /** @brief 批量入队（持锁一次），返回未入队的迭代器 */
    template <typename InIterator>
    inline InIterator pushSome(InIterator first, InIterator last)
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag)
        {
            return first;
        }
        for (; first != last && (m_capacity == 0 || m_queue.size() < m_capacity); ++first)
        {
            m_queue.push(*first);
        }
        return first;
    }

    inline bool pop(T& v)
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag || m_queue.empty())
            return false;

        pop(v, Order());
        return true;
    }

    /** @brief 最多弹出 count 个元素到输出迭代器 */
    template <typename OutIterator>
    inline OutIterator popSome(OutIterator oi, size_t count = size_t(-1))
    {
        LockGuard lock(m_mutex);
        if (m_stopFlag || m_queue.empty())
            return false;

        count = std::min(m_queue.size(), count);
        for (size_t i = 0; i < count; i++)
        {
            pop(*oi, Order());
            oi++;
        }
        return oi;
    }

    inline bool empty() const
    {
        LockGuard lock(m_mutex);
        return m_queue.empty();
    }

    inline bool full() const
    {
        LockGuard lock(m_mutex);
        return (m_capacity > 0 && m_queue.size() >= m_capacity) ? true : false;
    }

    inline size_t size() const
    {
        LockGuard lock(m_mutex);
        return m_queue.size();
    }

    inline size_t capacity() const { return m_capacity; }

    inline void stop()
    {
        LockGuard lock(m_mutex);
        m_stopFlag = true;
    }

private:
    template <typename Tag>
    bool pop(T& v, Tag tag);

    //template <>
    bool pop(T& v, ConcurrentQueueTraits::tagFIFO tag)
    {
        if (m_queue.empty())
            return false;
        v = m_queue.front();
        m_queue.pop();
        return true;
    }

    //template <>
    bool pop(T& v, ConcurrentQueueTraits::tagFILO tag)
    {
        if (m_queue.empty())
            return false;
        v = m_queue.top();
        m_queue.pop();
        return true;
    }

    //template <>
    bool pop(T& v, ConcurrentQueueTraits::tagPRIO tag)
    {
        if (m_queue.empty())
            return false;
        v = m_queue.top();
        m_queue.pop();
        return true;
    }

private:
    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue(ConcurrentQueue&&) = delete;
    void operator=(const ConcurrentQueue&) = delete;

    bool m_stopFlag;
    const size_t m_capacity;
    mutable MutexType m_mutex;
    QueueType m_queue;
};

template <typename T, typename Queue = std::queue<T>>
using FifoConcurrentQueue = ConcurrentQueue<T, Queue, ConcurrentQueueTraits::tagFIFO>;

template <typename T, typename Queue = std::stack<T>>
using FiloConcurrentQueue = ConcurrentQueue<T, Queue, ConcurrentQueueTraits::tagFILO>;

template <typename T, typename Queue = std::priority_queue<T>>
using PrioConcurrentQueue = ConcurrentQueue<T, Queue, ConcurrentQueueTraits::tagPRIO>;

} // namespace concurrency
} // namespace afl
