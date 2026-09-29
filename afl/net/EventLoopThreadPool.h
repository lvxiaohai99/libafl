/**
 * @file   EventLoopThreadPool.h
 * @brief  IO 事件循环线程池
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/net/SocketUtil.h"
#include "afl/net/InetAddress.h"
#include "afl/concurrency/Mutex.h"

namespace afl
{
namespace concurrency
{
class Thread;
class CountDownLatch;
} // namespace concurrency
} // namespace afl

namespace afl
{
namespace net
{
class EventLoop;
class EventLoopThreadPool : afl::base::NonCopy
{
public:
    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();

    std::vector<EventLoop*> getAllLoops() { return m_loops; }

    /// 设置EventLoopThreadPool的threads大小；if numThreads
    /// < 0  : 设置该为当前系统CPU并发数；
    /// == 0 : 不使用EventLoopThreadPool，所有Channel都在同一个EventLoop中运行，默认值；
    /// > 0  : 设置numThreads个线程，也即numThreads个EventLoop，每个连接择其中
    void setMultiReactorThreads(int numThreads);

    void start();
    bool isStart() { return m_started; }
    EventLoop* getNextLoop();

private:
    void runLoop();

private:
    EventLoop* m_baseLoop;
    bool m_started;
    int m_numThreads;
    size_t m_next;
    afl::concurrency::Mutex m_mutex;
    afl::concurrency::CountDownLatch* m_latch;
    std::vector<EventLoop*> m_loops;
    std::vector<afl::concurrency::Thread*> m_threads;
};

} // namespace net
} // namespace afl
