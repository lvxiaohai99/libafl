/**
 * @file   ThreadPool.cpp
 * @brief  线程池的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/concurrency/ThreadPool.h"
#include "afl/base/Exception.h"

#include <assert.h>
#include <exception>
#include "afl/concurrency/Thread.h"

namespace afl
{
namespace concurrency
{
ThreadPool::ThreadPool(const std::string& name /* = "ThreadPool"*/) : m_name(name), m_running(false)
{
}

ThreadPool::~ThreadPool()
{
    if (m_running)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    if (m_running)
        return;
    m_running = true;
    assert(m_threads.empty());
    m_threads.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
        char id[32];
        AFL_SNPRINTF(id, sizeof id, "%d", i);
        m_threads.push_back(new Thread(std::bind(&ThreadPool::executeThread, this), m_name + id));
    }
}

void ThreadPool::stop()
{
    m_running = false;
    m_queue.stop();
    for_each(m_threads.begin(), m_threads.end(), std::bind(&Thread::join, std::placeholders::_1));
}

void ThreadPool::run(const Task& task)
{
    if (m_threads.empty())
    {
        task();
    }
    else
    {
        m_queue.push(task);
    }
}

void ThreadPool::executeThread()
{
    try
    {
        while (m_running)
        {
            Task task(m_queue.pop());
            if (task)
            {
                /*bool ret = */ task();
            }
        }
    }
    catch (const afl::base::Exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", m_name.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        std::abort();
    }
    catch (const std::exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", m_name.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        std::abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", m_name.c_str());
        throw; // rethrow
    }
}

} // namespace concurrency
} // namespace afl
