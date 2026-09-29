/**
 * @file   Poller.h
 * @brief  I/O 多路复用抽象接口
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include <vector>
#include "afl/base/Common.h"
#include "afl/net/SocketUtil.h"
#include "afl/time/TimeStamp.h"
namespace afl
{
namespace net
{
#define POLL_WAIT_INDEFINITE

#define USE_POLLER_EPOLL
#define USE_POLLER_SELECT
#define USE_POLLER_POLL

using afl::time::TimeStamp;
class Socket;
class Channel;
class EventLoop;

class Poller
{
public:
    typedef std::vector<Channel*> ChannelList;
    typedef std::map<AFL_SOCKET, Channel*> ChannelMap;

public:
    explicit Poller(EventLoop* loop);
    virtual ~Poller();

    /// 根据各种宏定义及操作系统区分创建可用的backends
    /// @param loop        : EventLoop, I/O service
    /// @return 可用的 I/O 后端实现
    static Poller* createPoller(EventLoop* loop);

public:
    /// 添加/更新Channel绑定socket的I/O events, 必须在主循环中调
    /// @param channel     : 待更新的Channel
    /// @return            : 成功为true，失败为false
    virtual bool updateChannel(Channel* channel) = 0;

    /// 删除Channel绑定socket的I/O events, 必须在主循环中调
    /// @param channel     : 待删除的Channel
    /// @return            : 成功为true，失败为false
    virtual bool removeChannel(Channel* channel) = 0;

    /// 得到可响应读写事件的有连, 必须在主循环中调
    /// @param timeout     : 超时时间(单位:ms)
    /// @param activeConns : 宸叉縺娲荤殑杩炴帴
    /// @return            : io multiplexing 调用返回时的当前时间
    virtual TimeStamp pollOnce(int timeoutMs, ChannelList& activeChannels) = 0;

    /// 获得当前使用的IO复用backends的描
    /// @return            : IO 多路复用的名称
    virtual const char* ioMultiplexerName() const = 0;

public:
    /// 判断该Channel是否在Poller
    /// @param channel     : 待删除的Channel
    /// @return            : 存在为true，否则为false
    virtual bool hasChannel(const Channel* channel) const;

    /// 获取当前存在的连
    /// @param sock        : socket/timer/signal fd
    /// @return            : socket对应的连, 如不存在返回NULL
    virtual Channel* getChannel(AFL_SOCKET sock) const;

protected:
    ChannelMap m_channelMap;
    EventLoop* m_loop;
};

} // namespace net
} // namespace afl
