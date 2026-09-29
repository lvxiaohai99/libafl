/**
 * @file   EventLoop.cpp
 * @brief  IO 事件循环的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/EventLoop.h"

#include "afl/time/TimeStamp.h"
#include "afl/log/Log.h"
#include "afl/concurrency/Thread.h"
#include "afl/net/Channel.h"
#include "afl/net/poller/Poller.h"
#include "afl/net/TimerQueue.h"
#include "afl/net/EventFd.h"

#include <assert.h>
#include <sys/eventfd.h> // for eventfd
#include <signal.h>      // for ::signal

using namespace afl::concurrency;
namespace afl
{
namespace net
{
namespace
{
// 如果已关闭的client socket上继续write时，服务器进程会收到SIGPIPE信号而终,
// 这里单忽略该信号
class IgnoreSigPipe
{
public:
    IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
} _dont_use_this_class_;
} // namespace

thread_local EventLoop* g_evloopInThisThread = 0;

EventLoop::EventLoop()
    : m_currentThreadId(this_thread::tid()), m_currentActiveChannel(NULL), m_running(false),
      m_eventHandling(false), m_callingPendingFunctors(false), m_mutex()
{
    assert(
        !g_evloopInThisThread); // 避免在同个线程中创建多个EventLoop对象(one EventLoop per thread)

    m_poller = Poller::createPoller(this);

    m_wakeupfd = new EventfdHandler();
    m_wakeupChannel = new Channel(this, m_wakeupfd->fd());
    //m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    m_wakeupChannel->setReadCallback(
        std::bind(&EventfdHandler::read, m_wakeupfd, static_cast<uint64_t*>(NULL)));
    m_wakeupChannel->enableReading(); // ready for read event of m_wakeupfd

    m_timerQueue = new TimerQueue(this);

    g_evloopInThisThread = this;
}

EventLoop::~EventLoop()
{
    // 仅在 loop 线程上从 poller 摘除 channel；工作线程 EventLoop 常在主线程析构
    if (m_wakeupChannel)
    {
        if (isInLoopThread())
        {
            m_wakeupChannel->disableAll();
            m_wakeupChannel->remove();
        }
        SAFE_DELETE(m_wakeupChannel);
    }
    SAFE_DELETE(m_wakeupfd);
    SAFE_DELETE(m_poller);
    if (g_evloopInThisThread == this)
    {
        g_evloopInThisThread = nullptr;
    }
}

void EventLoop::loop()
{
    // 采纳调用线程为事件循环线程：允许在其它线程构造 EventLoop 而在此处启动
    m_currentThreadId = concurrency::this_thread::tid();
    m_running = true;

    TimeStamp now;
    while (m_running)
    {
        m_activeChannels.clear();

        int timeoutMs = 0;
        TimeStamp nextExpired = m_timerQueue->getNearestExpiration();
        if (nextExpired.valid())
        {
            now = TimeStamp::now();
            double seconds = TimeStamp::timeDiffS(nextExpired, now);
            LOG_DEBUG("nextExpired.valid() [%s][%s][%lf]", nextExpired.toString().c_str(),
                     now.toString().c_str(), seconds);
            if (seconds <= 0)
                timeoutMs = 0;
            else
                timeoutMs = seconds * 1000;
        }
        else
        {
#if defined(POLL_WAIT_INDEFINITE)
            timeoutMs = -1;
#else
            timeoutMs = 0;
#endif
        }

        now = m_poller->pollOnce(timeoutMs, m_activeChannels);
        LOG_DEBUG("EventLoop::loop [%s][%d]", now.toString().c_str(), m_activeChannels.size());

        m_eventHandling = true;
        for (ChannelList::iterator it = m_activeChannels.begin(); it != m_activeChannels.end();
             ++it)
        {
            m_currentActiveChannel = *it;
            m_currentActiveChannel->handleEvent(now);
        }
        m_currentActiveChannel = NULL;
        m_eventHandling = false;

        m_timerQueue->runTimer(now);

        callPendingFunctors(); //处理poll等待过程中发生的事件
    }
}

void EventLoop::quit()
{
    m_running = false;
    if (!isInLoopThread())
    {
        wakeupPoller();
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    LOG_DEBUG("EventLoop[%0x]::updateChannel [%d]", this, channel->fd());
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (m_eventHandling)
    {
        assert(m_currentActiveChannel == channel ||
               std::find(m_activeChannels.begin(), m_activeChannels.end(), channel) ==
                   m_activeChannels.end());
    }
    m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return m_poller->hasChannel(channel);
}

void EventLoop::runInLoop(const Functor& func)
{
    LOG_DEBUG("EventLoop[%0x]::runInLoop [%d][%0x]", this, isInLoopThread(), &func);
    if (isInLoopThread())
    {
        func();
    }
    else
    {
        queueInLoop(func);
    }
}

void EventLoop::queueInLoop(const Functor& func)
{
    LOG_DEBUG("EventLoop[%0x]::queueInLoop [%d][%0x]", this, isInLoopThread(), &func);
    {
        LockGuard<Mutex> lock(m_mutex);
        m_pendingFunctors.push_back(func);
    }

    if (!isInLoopThread() || !m_callingPendingFunctors) // may be should wakeup at any time
    {
        wakeupPoller();
    }
}

TimerId EventLoop::addTimer(const TimerCallback& cb, const TimeStamp& when)
{
    return m_timerQueue->addTimer(cb, when, 0);
}

TimerId EventLoop::addTimer(const TimerCallback& cb, double delaySeconds, bool repeat /* = false*/)
{
    TimeStamp when(TimeStamp::now());
    when += delaySeconds;
    return m_timerQueue->addTimer(cb, when, repeat ? delaySeconds : 0);
}

void EventLoop::cancelTimer(TimerId id)
{
    m_timerQueue->cancelTimer(id);
}

void EventLoop::callPendingFunctors()
{
    std::vector<Functor> tmp_functors;
    m_callingPendingFunctors = true;
    {
        LockGuard<Mutex> lock(m_mutex);
        tmp_functors.swap(m_pendingFunctors);
    }

    for (size_t i = 0; i < tmp_functors.size(); ++i)
    {
        tmp_functors[i]();
    }
    m_callingPendingFunctors = false;
}

void EventLoop::assertInLoopThread() const
{
    if (!isInLoopThread())
    {
        LOG_ALERT(
            "EventLoop::abortNotInLoopThread - EventLoop [%0x] was created in m_threadId [%d], "
            "but current thread id = [%d].",
            this, m_currentThreadId, this_thread::tid());
        assert("EventLoop::assertInLoopThread()" && 0);
    }
}

void EventLoop::wakeupPoller()
{
    LOG_DEBUG("EventLoop::wakeupPoller()");
    uint64_t value = 1;
    ssize_t n = m_wakeupfd->write(value);
    if (n != sizeof(value))
    {
        LOG_ERROR("EventLoop::wakeupPoller() m_wakeupfd write error[%d][%d]", n, errno);
    }
}

} // namespace net
} // namespace afl
