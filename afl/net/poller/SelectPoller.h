/**
 * @file   SelectPoller.h
 * @brief  I/O 多路复用 select 实现
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/net/poller/Poller.h"
#include <set>
#include <sys/select.h>
namespace afl
{
namespace net
{
class SelectPoller : public Poller
{
public:
    explicit SelectPoller(EventLoop* loop);

    ~SelectPoller();

public:
    virtual bool updateChannel(Channel* channel);

    virtual bool removeChannel(Channel* channel);

    virtual TimeStamp pollOnce(int timeoutMs, ChannelList& activeChannels);

    virtual const char* ioMultiplexerName() const { return "select"; }

private:
    void fireActiveChannels(int numEvents, ChannelList& activeChannels) const;

private:
    fd_set m_readfds;   /// select返回的所有可读事
    fd_set m_writefds;  /// select返回的所有可写事
    fd_set m_exceptfds; /// select返回的所有错误事

    fd_set m_selectReadFds;   /// 加入到select中的感兴趣的有可读事
    fd_set m_selectWriteFds;  /// 加入到select中的感兴趣的有可写事
    fd_set m_selectExceptFds; /// 加入到select中的感兴趣的有错误事

    std::set<int, std::greater<int>> m_fdlist;
};

} // namespace net
} // namespace afl
