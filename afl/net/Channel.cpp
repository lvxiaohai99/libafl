/**
 * @file   Channel.cpp
 * @brief  IO 事件通道的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/Channel.h"
#include "afl/net/EventLoop.h"
#include "afl/log/Log.h"

#include <sstream>
#include <assert.h>

namespace afl
{
namespace net
{
Channel::Channel(EventLoop* loop, int fd) : m_loop(loop), m_fd(fd), m_events(0), m_revents(0) {}

Channel::~Channel()
{
    if (m_loop->isInLoopThread())
    {
        // assert(!m_loop->hasChannel(this));
    }
}

void Channel::update()
{
    m_loop->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    m_loop->removeChannel(this);
}

void Channel::handleEvent(TimeStamp receiveTime)
{
    handleEventWithHold(receiveTime);
}

void Channel::handleEventWithHold(TimeStamp receiveTime)
{
    if ((m_revents & FDEVENT_HUP) && !(m_revents & FDEVENT_IN))
    {
        LOG_INFO("Channel::handleEventWithHold closeCallback, fd[%d]", m_fd);
        if (m_closeCallback)
            m_closeCallback();
    }

    if (m_revents & FDEVENT_NVAL)
    {
        LOG_WARN("Channel::handle_event() POLLNVAL, fd[%d]", m_fd);
    }

    if (m_revents & (FDEVENT_ERR | FDEVENT_NVAL))
    {
        LOG_INFO("Channel::handleEventWithHold closeCallback, fd[%d]", m_fd);
        if (m_errorCallback)
            m_errorCallback();
    }
    if (m_revents & kEventRead)
    {
        if (m_readCallback)
            m_readCallback(receiveTime);
    }
    if (m_revents & FDEVENT_OUT)
    {
        if (m_writeCallback)
            m_writeCallback();
    }
}

std::string Channel::reventsToString() const
{
    std::ostringstream oss;
    oss << m_fd << ": ";
    if (m_revents & FDEVENT_IN)
        oss << "IN ";
    if (m_revents & FDEVENT_PRI)
        oss << "PRI ";
    if (m_revents & FDEVENT_OUT)
        oss << "OUT ";
    if (m_revents & FDEVENT_HUP)
        oss << "HUP ";
    if (m_revents & FDEVENT_RDHUP)
        oss << "RDHUP ";
    if (m_revents & FDEVENT_ERR)
        oss << "ERR ";
    if (m_revents & FDEVENT_NVAL)
        oss << "NVAL ";

    return oss.str().c_str();
}

} // namespace net
} // namespace afl
