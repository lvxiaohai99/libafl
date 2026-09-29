/**
 * @file   PollPoller.h
 * @brief  I/O 多路复用 poll 实现
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/net/poller/Poller.h"
#include <unordered_map>
//#ifdef __GNUC__
//#include <ext/hash_map>
//#else
//#include <hash_map>
//#endif
//namespace std
//{
//    using namespace __gnu_cxx;
//}
namespace afl
{
namespace net
{
class PollPoller : public Poller
{
public:
    explicit PollPoller(EventLoop* loop);

    ~PollPoller();

public:
    virtual bool updateChannel(Channel* channel);

    virtual bool removeChannel(Channel* channel);

    virtual TimeStamp pollOnce(int timeoutMs, ChannelList& activeChannels);

    virtual const char* ioMultiplexerName() const { return "linux_poll"; }

private:
    void fireActiveChannels(int numEvents, ChannelList& activeChannels) const;

private:
    typedef std::vector<struct pollfd> PollFdList;
    typedef std::unordered_map<Channel*, int> ChannelIter;

    PollFdList m_pollfds;
    ChannelIter m_channelIter;
};

} // namespace net
} // namespace afl
