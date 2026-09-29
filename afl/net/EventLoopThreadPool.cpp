/**
 * @file   EventLoopThreadPool.cpp
 * @brief  IO 事件循环线程池的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/EventLoopThreadPool.h"
#include "afl/net/EventLoop.h"
#include "afl/concurrency/Thread.h"
#include "afl/concurrency/CountDownLatch.h"
#include "afl/string/StringUtil.h"
#include "afl/base/SmartAssert.h"
#include "afl/log/Log.h"

using afl::concurrency::Thread;

namespace afl
{
namespace net
{
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
    : m_baseLoop(baseLoop), m_started(false), m_numThreads(0), m_next(0), m_latch(NULL)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    std::for_each(m_loops.begin(), m_loops.end(),
                  std::bind(&EventLoop::quit, std::placeholders::_1));
    std::for_each(m_threads.begin(), m_threads.end(),
                  std::bind(&Thread::join, std::placeholders::_1));

    assert(m_loops.size() == m_threads.size());
    for (size_t i = 0; i < m_loops.size(); ++i)
    {
        SAFE_DELETE(m_threads[i]);
    }
}

void EventLoopThreadPool::setMultiReactorThreads(int numThreads)
{
    AFL_ASSERT(numThreads >= 0)(numThreads);
    if (numThreads < 0)
        numThreads = afl::concurrency::Thread::hardwareConcurrency();
    m_numThreads = numThreads;
    m_latch = new afl::concurrency::CountDownLatch(m_numThreads);
}

void EventLoopThreadPool::start()
{
    if (m_latch == NULL || m_numThreads < 0)
        setMultiReactorThreads(0);

    assert(!m_started);
    m_started = true;
    m_baseLoop->assertInLoopThread();

    for (int i = 0; i < m_numThreads; ++i)
    {
        std::string thrd_name = "m_eventloop";
        thrd_name += afl::str::toStr(i);
        Thread* thread = new Thread(std::bind(&EventLoopThreadPool::runLoop, this), thrd_name);
        m_threads.push_back(thread);
    }
    m_latch->wait();
    LOG_INFO("EventLoopThreadPool[%0x]::started [%ld][%d]", this,
             afl::concurrency::this_thread::tid(), m_numThreads);
}

void EventLoopThreadPool::runLoop()
{
    EventLoop this_loop;

    {
        afl::concurrency::LockGuard<afl::concurrency::Mutex> lock(m_mutex);
        m_loops.push_back(&this_loop);
    }
    //zl::thread::this_thread::sleepFor(zl::thread::chrono::seconds(2));
    m_latch->countDown();
    LOG_INFO("EventLoopThreadPool countDown [%0x]::runInLoop [%ld]", this,
             afl::concurrency::this_thread::tid());
    this_loop.loop();
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    assert(m_started);
    m_baseLoop->assertInLoopThread();

    EventLoop* loop = m_baseLoop;

    if (!m_loops.empty()) // round-robin
    {
        loop = m_loops[m_next];

        if (++m_next >= m_loops.size())
        {
            m_next = 0;
        }
    }

    return loop;
}

} // namespace net
} // namespace afl
