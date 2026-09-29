/**
 * @file   SignalFd.h
 * @brief  signalfd 封装；需 Linux 内核 ≥ 2.6.25
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/net/CallBacks.h"
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <signal.h>

namespace afl
{
namespace net
{
typedef int Signalfd;

/** @brief signalfd 信号集管理与回调分发 */
class SignalfdHandler
{
public:
    SignalfdHandler();
    ~SignalfdHandler();

public:
    Signalfd fd() { return m_signalFd; }

    void addSigHandler(int sig, const SignalCallback& handler);

    void removeSig(int sig);

    bool haveSignal(int sig);

    /** @brief 注册全部已添加的信号；须在 addSigHandler 之后调用 */
    void registerAll(int flags = SFD_NONBLOCK | SFD_CLOEXEC);

    /** @brief 读取一次信号并触发回调，返回信号编号 */
    int readSig();

    /** @brief 同步循环读取直至 stop */
    void wait();

    void stop() { m_isReady = false; }

private:
    Signalfd createSignalfd(int flags);

private:
    typedef std::unordered_map<int, SignalCallback> SigHandlerMap;

    bool m_isReady;
    Signalfd m_signalFd;
    sigset_t m_mask;
    SigHandlerMap m_sigHandlers;
};

} // namespace net
} // namespace afl
