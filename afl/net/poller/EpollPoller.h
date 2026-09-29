/**
 * @file   EpollPoller.h
 * @brief  I/O 多路复用 epoll 实现
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/net/poller/Poller.h"
struct epoll_event;
namespace afl
{
namespace net
{
class EpollPoller : public Poller
{
public:
    explicit EpollPoller(EventLoop* loop, bool enableET = false);

    ~EpollPoller();

public:
    virtual bool updateChannel(Channel* channel);

    virtual bool removeChannel(Channel* channel);

    virtual TimeStamp pollOnce(int timeoutMs, ChannelList& activeChannels);

    virtual const char* ioMultiplexerName() const { return "linux_epoll"; }

private:
    bool update(Channel* channel, int operation);

    void fireActiveChannels(int numEvents, ChannelList& activeChannels) const;

private:
    typedef std::vector<struct epoll_event> EpollEventList;

    int m_epollfd;
    bool m_enableET;
    EpollEventList m_events;
};

} // namespace net
} // namespace afl
