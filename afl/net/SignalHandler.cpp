/**
 * @file   SignalHandler.cpp
 * @brief  基于 EventLoop 的 signalfd 信号分发实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/SignalHandler.h"
#include "afl/log/Log.h"
#include "afl/net/Channel.h"
#include "afl/concurrency/Semaphore.h"
#include <signal.h>
#include <sys/signal.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

namespace afl
{
namespace net
{
SignalHandler::SignalHandler(afl::net::EventLoop& loop)
    : m_loop(loop), m_sigChannel(nullptr), m_signalFd(-1)
{
    sigemptyset(&m_sigMask);

    m_signalFd = signalfd(-1, &m_sigMask, SFD_NONBLOCK);
    if (m_signalFd < 0)
    {
        LOG_ERROR("SignalHandler: signalfd create failed errno=%d(%s)", errno, strerror(errno));
        m_sigChannel = nullptr;
        return;
    }

    m_sigChannel = new afl::net::Channel(&m_loop, m_signalFd);
    m_sigChannel->setReadCallback(std::bind(&SignalHandler::procSignals, this));

    m_loop.runInLoop([this]() {
        if (m_sigChannel)
        {
            m_sigChannel->enableReading();
        }
    });
    LOG_INFO("SignalHandler: created fd=%d", m_signalFd);
}

SignalHandler::~SignalHandler()
{
    m_sigHandlers.clear();

    if (m_signalFd < 0 && !m_sigChannel)
    {
        return;
    }

    // 先从 epoll 摘掉 channel，再关 fd，避免 update 已关闭的 socket
    m_loop.runInLoop([this]() {
        if (m_sigChannel)
        {
            m_sigChannel->disableAll();
            m_sigChannel->remove();
            SAFE_DELETE(m_sigChannel);
        }
        if (m_signalFd >= 0)
        {
            ::close(m_signalFd);
            m_signalFd = -1;
        }
    });
    LOG_DEBUG("SignalHandler: destroyed");
}

bool SignalHandler::addSignal(int sigo, SignalCB cb)
{
    if (m_signalFd < 0)
    {
        LOG_ERROR("SignalHandler::addSignal(%d) failed: signalfd not ready", sigo);
        return false;
    }

    bool retb = true;
    afl::concurrency::Semaphore sema;

    m_loop.runInLoop([&]() {
        if (sigaddset(&m_sigMask, sigo) < 0 || sigprocmask(SIG_BLOCK, &m_sigMask, NULL) < 0 ||
            signalfd(m_signalFd, &m_sigMask, SFD_NONBLOCK) < 0)
        {
            retb = false;
            LOG_ERROR("SignalHandler::addSignal(%d) setup failed errno=%d(%s)", sigo, errno,
                      strerror(errno));
        }
        else
        {
            m_sigHandlers[sigo] = cb;
            LOG_INFO("SignalHandler::addSignal(%d) ok", sigo);
        }

        sema.post();
    });

    sema.wait();
    return retb;
}

void SignalHandler::procSignals()
{
    struct signalfd_siginfo siginfo;
    int rlen = ::read(m_signalFd, &siginfo, sizeof(siginfo));
    if (rlen != (int)sizeof(siginfo))
    {
        LOG_ERROR("SignalHandler::procSignals read failed n=%d errno=%d(%s)", rlen, errno,
                  strerror(errno));
        return;
    }

    LOG_INFO("SignalHandler: received signo=%d", (int)siginfo.ssi_signo);
    auto hdl = m_sigHandlers.find(siginfo.ssi_signo);
    if (hdl != m_sigHandlers.end())
    {
        hdl->second(siginfo);
    }
    else
    {
        LOG_WARN("SignalHandler: no callback for signo=%d", (int)siginfo.ssi_signo);
    }
}

} // namespace net
} // namespace afl
