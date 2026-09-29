/**
 * @file   TcpAcceptor.h
 * @brief  服务端接受器，监听某一端口，接受远程 socket 连接
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/time/TimeStamp.h"
#include "afl/base/NonCopy.h"

namespace afl
{
namespace net
{
class Socket;
class InetAddress;
class Channel;
class EventLoop;
using afl::time::TimeStamp;

/** @brief 监听端口并接受新 TCP 连接 */
class TcpAcceptor : afl::base::NonCopy
{
public:
    /** @brief 新连接回调：(fd, 对端地址) */
    typedef std::function<void(int, const InetAddress&)> NewConnectionCallback;

public:
    TcpAcceptor(EventLoop* loop, const InetAddress& listenAddr);
    ~TcpAcceptor();

    void setNewConnectionCallback(const NewConnectionCallback& callback)
    {
        m_newConnCallBack = callback;
    }

    void listen();

private:
    void onAccept(TimeStamp now);

private:
    EventLoop* m_loop;
    Socket* m_acceptSocket;
    Channel* m_acceptChannel;
    NewConnectionCallback m_newConnCallBack;
};

} // namespace net
} // namespace afl
