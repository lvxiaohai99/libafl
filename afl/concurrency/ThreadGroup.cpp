/**
 * @file   ThreadGroup.cpp
 * @brief  线程组管理的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/concurrency/ThreadGroup.h"
#include "afl/concurrency/Thread.h"
#include <assert.h>

namespace afl
{
namespace concurrency
{
ThreadGroup::ThreadGroup() {}

ThreadGroup::~ThreadGroup()
{
    for (auto it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
    {
        delete *it;
    }
    m_threads.clear();
}

bool ThreadGroup::isThisThreadIn()
{
    Thread::id id = this_thread::getId();
    LockGuard<Mutex> lock(m_mutex);
    for (auto it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
    {
        if ((*it)->getId() == id)
            return true;
    }
    return false;
}

bool ThreadGroup::is_thread_in(Thread* thrd)
{
    if (thrd)
    {
        Thread::id id = thrd->getId();
        LockGuard<Mutex> lock(m_mutex);
        for (auto it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
        {
            if ((*it)->getId() == id)
                return true;
        }
        return false;
    }

    return false;
}

void ThreadGroup::addThread(Thread* thrd)
{
    if (thrd)
    {
        assert(!is_thread_in(thrd) && "must not add a duplicated thread");
        LockGuard<Mutex> lock(m_mutex);
        m_threads.push_back(thrd);
    }
}

void ThreadGroup::removeThread(Thread* thd)
{
    LockGuard<Mutex> lock(m_mutex);
    std::vector<Thread*>::iterator it = std::find(m_threads.begin(), m_threads.end(), thd);
    if (it != m_threads.end())
    {
        m_threads.erase(it);
    }
}

void ThreadGroup::joinAll()
{
    assert(!isThisThreadIn() && "trying joining itself");
    LockGuard<Mutex> lock(m_mutex);
    //for_each(m_threads.begin(), m_threads.end(), std::bind(&Thread::join, std::placeholders::_1));
    for (auto it = m_threads.begin(), end = m_threads.end(); it != end; ++it)
    {
        if ((*it)->joinable())
            (*it)->join();
    }
}

size_t ThreadGroup::size() const
{
    LockGuard<Mutex> lock(m_mutex);
    return m_threads.size();
}

} // namespace concurrency
} // namespace afl
