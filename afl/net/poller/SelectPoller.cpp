/**
 * @file   SelectPoller.cpp
 * @brief  I/O 多路复用 select 的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/poller/SelectPoller.h"
#include "afl/net/Channel.h"
#include "afl/log/Log.h"
namespace afl
{
namespace net
{
SelectPoller::SelectPoller(EventLoop* loop) : Poller(loop)
{
    FD_ZERO(&m_readfds);
    FD_ZERO(&m_writefds);
    FD_ZERO(&m_exceptfds);
    FD_ZERO(&m_selectReadFds);
    FD_ZERO(&m_selectWriteFds);
    FD_ZERO(&m_selectExceptFds);
}

SelectPoller::~SelectPoller() {}

bool SelectPoller::updateChannel(Channel* channel)
{
    AFL_SOCKET fd = channel->fd();
    LOG_INFO("SelectPoller::updateChannel[%d]", fd);

    FD_SET(fd, &m_exceptfds);

    int events = channel->events();
    if (hasChannel(channel)) //exist, update
    {
        assert(getChannel(fd) == channel);

        if (events & FDEVENT_IN)
            FD_SET(fd, &m_selectReadFds);
        else
            FD_CLR(fd, &m_selectReadFds);

        if (events & FDEVENT_OUT)
            FD_SET(fd, &m_selectWriteFds);
        else
            FD_CLR(fd, &m_selectWriteFds);

        if (channel->isNoneEvent())
        {
            LOG_INFO("SelectPoller::updateChannel [%d][%0x] NoneEvent", fd, channel);
            m_fdlist.erase(fd);
        }
    }
    else //new, add
    {
        assert(getChannel(fd) == NULL);

        if (events & FDEVENT_IN)
            FD_SET(fd, &m_selectReadFds);
        if (events & FDEVENT_OUT)
            FD_SET(fd, &m_selectWriteFds);

        m_channelMap[fd] = channel;
        m_fdlist.insert(fd);
        //m_channelIter.insert(std::make_pair(channel, m_pollfds.size() - 1));
    }
    return true;
}

bool SelectPoller::removeChannel(Channel* channel)
{
    if (!hasChannel(channel))
        return true;

    AFL_SOCKET fd = channel->fd();
    LOG_INFO("SelectPoller::removeChannel [%d][%0x]", fd, channel);
    assert(hasChannel(channel) && "the remove socket must be already exist");
    assert(getChannel(fd) == channel && "the remove socket must be already exist");
    assert(channel->isNoneEvent());
    size_t n = m_channelMap.erase(fd);
    AFL_UNUSED(n);
    assert(n == 1);

    m_fdlist.erase(fd);

    FD_CLR(fd, &m_selectReadFds);
    FD_CLR(fd, &m_selectWriteFds);
    FD_CLR(fd, &m_selectExceptFds);

    return true;
}

TimeStamp SelectPoller::pollOnce(int timeoutMs, ChannelList& activeChannels)
{
    struct timeval tv = {0, 0};
    struct timeval* ptv = &tv;
    if (timeoutMs < 0)
        ptv = NULL;
    else
    {
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
    }

    m_readfds = m_selectReadFds;
    m_writefds = m_selectWriteFds;
    m_exceptfds = m_selectExceptFds;

    //LOG_INFO("SelectPoller::pollOnce: [%d][%d]", *m_fdlist.begin(), timeoutMs);
    int numEvents = ::select(*m_fdlist.begin() + 1, &m_readfds, &m_writefds, &m_exceptfds, ptv);
    int savedErrno = errno;
    TimeStamp now(TimeStamp::now());
    if (numEvents > 0)
    {
        LOG_INFO("SelectPoller::pollOnce: events happended[%d]", numEvents);
        fireActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0)
    {
        LOG_INFO("SelectPoller::pollOnce: nothing happended");
    }
    else
    {
        if (savedErrno != SOCK_ERR_EINTR)
        {
            errno = savedErrno;
            LOG_INFO("SelectPoller::pollOnce: error [%d]", errno);
        }
    }
    return now;
}

void SelectPoller::fireActiveChannels(int numEvents, ChannelList& activeChannels) const
{
    //for (AFL_SOCKET fd = 0; numEvents > 0 && fd <= m_maxFd ; ++fd) //可优化，此处再单独保存一个fd的集合即
    for (auto it = m_fdlist.begin(); numEvents > 0 && it != m_fdlist.end(); ++it)
    {
        AFL_SOCKET fd = *it;

        int revents = FDEVENT_NONE;
        if (FD_ISSET(fd, &m_readfds))
            revents |= FDEVENT_IN;
        if (FD_ISSET(fd, &m_writefds))
            revents |= FDEVENT_OUT;
        if (FD_ISSET(fd, &m_exceptfds))
            revents |= FDEVENT_ERR;

        if (revents != FDEVENT_NONE)
        {
            --numEvents;
            Channel* channel = getChannel(fd);
            assert(channel && "the channel must be already exist");

            channel->setRevents(revents);
            activeChannels.push_back(channel);
        }
    }
}

} // namespace net
} // namespace afl
