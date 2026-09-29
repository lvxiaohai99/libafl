/**
 * @file   Channel.h
 * @brief  IO 事件通道，封装 fd 及其关注的事件与回调
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/SocketUtil.h"
#include "afl/time/TimeStamp.h"
#include "afl/base/NonCopy.h"

namespace afl
{
namespace net
{
class EventLoop;
using afl::time::TimeStamp;

// 可 poll 的事件位：写入 events 表示关注项，在 revents 中反映 fd 状态
#define FDEVENT_NONE 0x000 /* 无事件 */

// 与 Linux poll.h 中 POLL* 对应
#define FDEVENT_IN 0x001  /* 可读 */
#define FDEVENT_PRI 0x002 /* 带外/紧急可读 */
#define FDEVENT_OUT 0x004 /* 可写且不阻塞 */

// 内核常隐式返回的状态位，不必写入 events
#define FDEVENT_ERR 0x008  /* 错误 */
#define FDEVENT_HUP 0x010  /* 对端挂断 */
#define FDEVENT_NVAL 0x020 /* poll 请求无效 */

#define FDEVENT_RDHUP 0x2000 /* GNU 扩展：对端关闭写端 */

enum
{
    kEventNone = FDEVENT_NONE,
    kEventRead = FDEVENT_IN | FDEVENT_PRI,
    kEventWrite = FDEVENT_OUT,
    kEventError = FDEVENT_ERR
};

class Channel : afl::base::NonCopy
{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(TimeStamp)> ReadEventCallback;

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

public:
    int fd() const { return m_fd; }

    EventLoop* ownerLoop() { return m_loop; }

    void setReadCallback(const ReadEventCallback& cb) { m_readCallback = cb; }

    void setWriteCallback(const EventCallback& cb) { m_writeCallback = cb; }

    void setCloseCallback(const EventCallback& cb) { m_closeCallback = cb; }

    void setErrorCallback(const EventCallback& cb) { m_errorCallback = cb; }

    int events() const { return m_events; }

    void setRevents(int revt) { m_revents = revt; }

    int revents() const { return m_revents; }

    void enableReading()
    {
        m_events |= kEventRead;
        update();
    }

    void disableReading()
    {
        m_events &= ~kEventRead;
        update();
    }

    void enableWriting()
    {
        m_events |= kEventWrite;
        update();
    }

    void disableWriting()
    {
        m_events &= ~kEventWrite;
        update();
    }

    void disableAll()
    {
        m_events = kEventNone;
        update();
    }

    bool isNoneEvent() const { return m_events == kEventNone; }

    bool isWriting() const { return m_events & kEventWrite; }

    void handleEvent(TimeStamp receiveTime);
    void remove();
    std::string reventsToString() const;

private:
    void update();
    void handleEventWithHold(TimeStamp receiveTime);

private:
    EventLoop* m_loop;
    int m_fd; // m_fd may be socket\signal\timerfd
    int m_events;
    int m_revents; // events of the poller returned

    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};

} // namespace net
} // namespace afl
