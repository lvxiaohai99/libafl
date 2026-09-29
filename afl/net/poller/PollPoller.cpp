/**
 * @file   PollPoller.cpp
 * @brief  I/O 多路复用 poll 的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/poller/PollPoller.h"
#include "afl/net/Channel.h"
#include "afl/log/Log.h"
namespace afl
{
namespace net
{
PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {}

PollPoller::~PollPoller() {}

bool PollPoller::updateChannel(Channel* channel)
{
    AFL_SOCKET fd = channel->fd();
    LOG_INFO("PollPoller::updateChannel[%d]", fd);

    int pevents = FDEVENT_NONE;
    int events = channel->events();
    if (events & FDEVENT_IN)
        pevents |= POLLIN;
    if (events & FDEVENT_OUT)
        pevents |= POLLOUT;
    pevents |= POLLERR | POLLHUP;

    if (m_channelIter.find(channel) != m_channelIter.end()) //exist, update
    {
        assert(getChannel(fd) == channel);
        int idx = m_channelIter[channel];
        LOG_INFO("PollPoller::updateChannel  fd2 [%d][%d]", idx, m_channelIter[channel]);
        assert(0 <= idx && idx < static_cast<int>(m_pollfds.size()));

        struct pollfd& pfd = m_pollfds[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.events = static_cast<short>(pevents);
        pfd.revents = 0;
        if (channel->isNoneEvent())
        {
            LOG_INFO("PollPoller::updateChannel [%d][%0x][%d] NoneEvent", fd, channel, pfd.events);
            pfd.fd = -channel->fd() - 1;
        }
    }
    else //new, add
    {
        assert(getChannel(fd) == NULL);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = static_cast<short>(pevents);
        pfd.revents = 0;
        m_pollfds.push_back(pfd);

        m_channelMap[fd] = channel;
        m_channelIter.insert(std::make_pair(channel, m_pollfds.size() - 1));
    }
    return true;
}

bool PollPoller::removeChannel(Channel* channel)
{
    if (!hasChannel(channel))
        return true;

    AFL_SOCKET fd = channel->fd();
    int idx = m_channelIter[channel];
    LOG_INFO("PollPoller::removeChannel [%d][%d][%0x]", fd, idx, channel);
    assert(getChannel(fd) == channel && "the remove socket must be already exist");
    assert(channel->isNoneEvent());
    assert(0 <= idx && idx < static_cast<int>(m_pollfds.size()));

    const struct pollfd& pfd = m_pollfds[idx];
    AFL_UNUSED(pfd);
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());

    size_t n = m_channelMap.erase(fd);
    AFL_UNUSED(n);
    assert(n == 1);
    if ((idx) == static_cast<int>(m_pollfds.size()) - 1) // last one
    {}
    else
    {
        int lastfd = m_pollfds.back().fd;
        iter_swap(m_pollfds.begin() + idx, m_pollfds.end() - 1);
        if (lastfd < 0)
        {
            lastfd = -lastfd - 1;
        }
        m_channelIter[getChannel(lastfd)] = idx;
    }

    m_pollfds.pop_back();
    m_channelIter.erase(channel);

    return true;
}

TimeStamp PollPoller::pollOnce(int timeoutMs, ChannelList& activeChannels)
{
    int numEvents = ::poll(&*m_pollfds.begin(), m_pollfds.size(), timeoutMs);
    int savedErrno = errno;
    TimeStamp now(TimeStamp::now());
    if (numEvents > 0)
    {
        fireActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0)
    {
        LOG_INFO("PollPoller::pollOnce: nothing happended");
    }
    else
    {
        if (savedErrno != SOCK_ERR_EINTR)
        {
            errno = savedErrno;
            LOG_INFO("EpollPoller::pollOnce: error [%d]", errno);
        }
    }
    return now;
}

void PollPoller::fireActiveChannels(int numEvents, ChannelList& activeChannels) const
{
    for (PollFdList::const_iterator it = m_pollfds.begin(); numEvents > 0 && it != m_pollfds.end();
         ++it)
    {
        if (it->revents > 0)
        {
            --numEvents;
            Channel* channel = getChannel(it->fd);
            assert(channel && "the channel must be already exist");
            //channel->setRevents(it->revents);
            int revents = FDEVENT_NONE;
            if (it->revents & POLLIN)
                revents |= FDEVENT_IN;
            if (it->revents & POLLPRI)
                revents |= FDEVENT_PRI;
            if (it->revents & POLLOUT)
                revents |= FDEVENT_OUT;
            if (it->revents & POLLERR)
                revents |= FDEVENT_ERR;
            if (it->revents & POLLHUP)
                revents |= FDEVENT_HUP;
            if (it->revents & POLLNVAL)
                revents |= FDEVENT_NVAL; // never happen
            channel->setRevents(revents);

            activeChannels.push_back(channel);
            //LOG_INFO("PollPoller::fireActiveChannels [%d][%d]", it->fd, it->revents);
        }
    }
}

} // namespace net
} // namespace afl
