/**
 * @file   EventLoop.h
 * @brief  IO 事件循环（io service），可管理 socket、timer、signal 等 IO 事件
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include "afl/concurrency/Thread.h"
#include "afl/concurrency/Atomic.h"
#include "afl/net/CallBacks.h"

namespace afl
{
namespace net
{
class Channel;
class Poller;
class Timer;
class TimerQueue;
class EventfdHandler;

class EventLoop : afl::base::NonCopy
{
public:
    typedef std::function<void()> Functor;

public:
    EventLoop();
    ~EventLoop();

    /// 启动事件循环；调用本方法的线程将被采纳为事件循环线程
    ///（允许在其它线程构造 EventLoop、而在 loop() 线程中运行）
    void loop();
    void quit();

public:
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    /// 在主线程中运行，如果是其他线程调用，则转为调用queueInLoop
    /// @param func        : 待运行事
    void runInLoop(const Functor& func);

    /// 异步调用：将操作存入待处理队列，等待 poller 唤醒后统一执行
    /// @param func        : 待运行事
    void queueInLoop(const Functor& func);

    TimerId addTimer(const TimerCallback& cb, const TimeStamp& when);
    TimerId addTimer(const TimerCallback& cb, double delaySeconds, bool repeat = false);
    void cancelTimer(TimerId id);

    bool isRunning() { return m_running; }
    bool isInLoopThread() const { return m_currentThreadId == concurrency::this_thread::tid(); }

    int getCurrentThreadId() const { return m_currentThreadId; }

    void assertInLoopThread() const;

private:
    void wakeupPoller();        //wakeup the waiting poller
    void callPendingFunctors(); //call when loop() return

private:
    typedef std::vector<Channel*> ChannelList;

    int m_currentThreadId; // thread id running the event loop (set when loop() starts)

    ChannelList m_activeChannels;              // active channels when poll return
    Channel* m_currentActiveChannel;           // the current processing active channel
    Poller* m_poller;                          // I/O poller
    concurrency::Atomic<bool> m_running;       // status for eventloop running
    concurrency::Atomic<bool> m_eventHandling; // status for active channel handling

    EventfdHandler* m_wakeupfd; // wakeup poller::poll
    Channel* m_wakeupChannel;   // channel of m_wakeupfd

    concurrency::Atomic<bool> m_callingPendingFunctors; // status for pending functors calling
    concurrency::Mutex m_mutex;                         // for guard  m_pendingFunctors
    std::vector<Functor> m_pendingFunctors;             // functors when polling, need mutex guard

    TimerQueue* m_timerQueue;
};

} // namespace net
} // namespace afl
