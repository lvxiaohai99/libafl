/**
 * @file   SignalHandler.h
 * @brief  基于 EventLoop 的 signalfd 信号分发
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/CallBacks.h"
#include "afl/net/EventLoop.h"
#include "afl/net/Channel.h"
#include <sys/signalfd.h>
#include <unordered_map>
#include <signal.h>
#include <sys/epoll.h>

namespace afl
{
namespace net
{
class SignalHandler
{
public:
    using SigInfo = struct signalfd_siginfo;
    using SignalCB = std::function<void(SigInfo&)>;

private:
    using SignalHandlers = std::unordered_map<int, SignalCB>;

public:
    SignalHandler(afl::net::EventLoop& loop);
    virtual ~SignalHandler();

    bool addSignal(int sigo, SignalCB cb);

private:
    void procSignals();

private:
    afl::net::EventLoop& m_loop;
    afl::net::Channel* m_sigChannel;
    int m_signalFd;
    sigset_t m_sigMask;
    SignalHandlers m_sigHandlers;
};


} // namespace net
} // namespace afl
