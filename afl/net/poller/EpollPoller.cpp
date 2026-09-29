/**
 * @file   EpollPoller.cpp
 * @brief  I/O 多路复用 epoll 的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/poller/EpollPoller.h"
#include "afl/net/Socket.h"
#include "afl/net/Channel.h"
#include "afl/log/Log.h"
#include "afl/base/SmartAssert.h"
#include <string.h>
#include <sys/epoll.h>
namespace afl
{
namespace net
{
AFL_STATIC_ASSERT(EPOLLIN == POLLIN, "must equal");
AFL_STATIC_ASSERT(EPOLLPRI == POLLPRI, "must equal");
AFL_STATIC_ASSERT(EPOLLOUT == POLLOUT, "must equal");
AFL_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP, "must equal");
AFL_STATIC_ASSERT(EPOLLERR == POLLERR, "must equal");
AFL_STATIC_ASSERT(EPOLLHUP == POLLHUP, "must equal");

EpollPoller::EpollPoller(EventLoop* loop, bool enableET /* = false*/)
    : Poller(loop), m_enableET(enableET), m_events(64)
{
    m_epollfd = epoll_create(1024);
    assert(m_epollfd > 0 && " epoll create failure!");
}

EpollPoller::~EpollPoller()
{
    ::close(m_epollfd);
}

bool EpollPoller::updateChannel(Channel* channel)
{
    AFL_SOCKET fd = channel->fd();
    LOG_DEBUG("EpollPoller::updateChannel[%d]", fd);
    if (hasChannel(channel)) //exist, update
    {
        assert(getChannel(fd) == channel);
        if (channel->isNoneEvent())
        {
            LOG_DEBUG("EpollPoller::updateChannel [%d][%0x] NoneEvent", fd, channel);
            m_channelMap.erase(fd);
            return update(channel, EPOLL_CTL_DEL);
        }
        else
        {
            return update(channel, EPOLL_CTL_MOD);
        }
    }
    else //new, add
    {
        assert(getChannel(fd) == NULL);
        m_channelMap[fd] = channel;
        return update(channel, EPOLL_CTL_ADD);
    }
}

bool EpollPoller::removeChannel(Channel* channel)
{
    if (!hasChannel(channel)) // 注意 updateChannel 函数中也有一处removeChannel的辑
        return true;

    AFL_SOCKET fd = channel->fd();
    LOG_DEBUG("EpollPoller::removeChannel [%d][%0x]", fd, channel);
    assert(hasChannel(channel) && "the remove socket must be already exist");
    assert(getChannel(fd) == channel && "the remove socket must be already exist");
    assert(channel->isNoneEvent());
    size_t n = m_channelMap.erase(fd);
    AFL_UNUSED(n);
    assert(n == 1);

    return update(channel, EPOLL_CTL_DEL);
}

bool EpollPoller::update(Channel* channel, int operation)
{
    AFL_SOCKET fd = channel->fd();
    struct epoll_event ev = {0, {0}};
    //ev.events = channel->events();
    int events = channel->events();
    if (events & FDEVENT_IN)
        ev.events |= EPOLLIN;
    if (events & FDEVENT_OUT)
        ev.events |= EPOLLOUT;
    ev.events |= EPOLLERR | EPOLLHUP;

    if (m_enableET)
        ev.events |= EPOLLET;

    ev.data.ptr = channel;

    if (::epoll_ctl(m_epollfd, operation, fd, &ev) < 0)
    {
        LOG_CRITICAL("EpollPoller::update error, [socket %d][op %d]", fd, operation);
        return false;
    }
    return true;
}

TimeStamp EpollPoller::pollOnce(int timeoutMs, ChannelList& activeChannels)
{
    int numEvents =
        ::epoll_wait(m_epollfd, &*m_events.begin(), static_cast<int>(m_events.size()), timeoutMs);
    int savedErrno = errno;
    TimeStamp now(TimeStamp::now());
    if (numEvents > 0)
    {
        LOG_DEBUG("EpollPoller::pollOnce: [%d] events happended", numEvents);
        fireActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("EpollPoller::pollOnce: nothing happended");
    }
    else
    {
        // 记录非常见错误
        // TODO：EINTR 时应返回 -1，其它错误返回 0
        if (savedErrno != SOCK_ERR_EINTR)
        {
            errno = savedErrno;
            LOG_DEBUG("EpollPoller::pollOnce: error [%d]", savedErrno);
        }
    }

    return now;
}

void EpollPoller::fireActiveChannels(int numEvents, ChannelList& activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= m_events.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(m_events[i].data.ptr);
        assert(hasChannel(channel) && "the channel must be already exist");

        //channel->setRevents(m_events[i].events);
        int revents = FDEVENT_NONE;
        if (m_events[i].events & EPOLLIN)
            revents |= FDEVENT_IN;
        if (m_events[i].events & EPOLLPRI)
            revents |= FDEVENT_PRI;
        if (m_events[i].events & EPOLLOUT)
            revents |= FDEVENT_OUT;
        if (m_events[i].events & EPOLLERR)
            revents |= FDEVENT_ERR;
        if (m_events[i].events & EPOLLHUP)
            revents |= FDEVENT_HUP;
        channel->setRevents(revents);

        activeChannels.push_back(channel);
    }
}

} // namespace net
} // namespace afl
