/**
 * @file   Poller.cpp
 * @brief  I/O 多路复用抽象接口的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/poller/Poller.h"
#include "afl/net/poller/EpollPoller.h"
#include "afl/net/poller/PollPoller.h"
#include "afl/net/poller/SelectPoller.h"
#include "afl/net/Channel.h"
namespace afl
{
namespace net
{
Poller::Poller(EventLoop* loop) : m_loop(loop) {}

Poller::~Poller() {}

bool Poller::hasChannel(const Channel* channel) const
{
    ChannelMap::const_iterator itr = m_channelMap.find(channel->fd());
    return itr != m_channelMap.end() && itr->second == channel;
}

Channel* Poller::getChannel(AFL_SOCKET sock) const
{
    ChannelMap::const_iterator itr = m_channelMap.find(sock);
    if (itr == m_channelMap.end())
        return NULL;
    return itr->second;
}

/*static*/ Poller* Poller::createPoller(EventLoop* loop)
{
#if defined(USE_POLLER_EPOLL)
    return new EpollPoller(loop);
#elif defined(USE_POLLER_SELECT)
    return new SelectPoller(loop);
#elif defined(USE_POLLER_POLL)
    return new PollPoller(loop);
#else
    return NULL;
#endif
}

} // namespace net
} // namespace afl
