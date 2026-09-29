/**
 * @file   ObjectPool.h
 * @brief  简单的对象池实现（支持单/多线程，可配置固定类型的分配数量）
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/concurrency/Mutex.h"
#include "afl/base/Common.h"

#include <assert.h>
#include <list>
#include <vector>

namespace afl
{
namespace base
{
template <typename T, class LockType = afl::concurrency::Mutex>
class ObjectPool
{
    enum
    {
        size_per_alloc = 128
    };

public:
    explicit ObjectPool(int preAllocNum = 4, int maxAllocNum = 0)
    {
        m_totalAllocNum = 0;
        m_maxAllocNum = maxAllocNum;
        pre_alloc(preAllocNum <= 0 ? size_per_alloc : preAllocNum);
    }

    ~ObjectPool()
    {
        for (size_t i = 0; i < m_chunks.size(); i++)
        {
            delete[] m_chunks[i];
        }
    }

public:
    T* alloc()
    {
        afl::concurrency::LockGuard<LockType> lock(m_mutex);
        if (m_pools.empty() && !pre_alloc(size_per_alloc))
        {
            return NULL;
        }

        T* t = m_pools.front();
        m_pools.pop_front();
        return t;
    }

    void free(T* t)
    {
        afl::concurrency::LockGuard<LockType> lock(m_mutex);
        m_pools.push_front(t);
    }

    int max_alloc() const { return m_maxAllocNum; }

    int total() const
    {
        afl::concurrency::LockGuard<LockType> lock(m_mutex);
        return m_totalAllocNum;
    }

    int avail() const
    {
        afl::concurrency::LockGuard<LockType> lock(m_mutex);
        return m_pools.size();
    }

private:
    bool pre_alloc(int allocNum)
    {
        if (m_maxAllocNum > 0)
            allocNum = allocNum < (m_maxAllocNum - m_totalAllocNum)
                           ? allocNum
                           : (m_maxAllocNum - m_totalAllocNum);
        if (allocNum <= 0)
            return false;

        T* chunk = new T[allocNum];
        m_chunks.push_back(chunk);

        for (int i = 0; i < allocNum; i++)
        {
            T* t = &chunk[i];
            m_pools.push_back(t);
        }

        m_totalAllocNum += allocNum;
        return true;
    }

private:
    mutable LockType m_mutex; // guard(single thread or multithread)
    int m_maxAllocNum;        // max alloc num of T, 0 means infinite
    int m_totalAllocNum;      // total alloc num of T
    std::list<T*> m_pools;    // available instance of T
    std::vector<T*> m_chunks; // save all memory from "new"
};

} // namespace base
} // namespace afl
