/**
 * @file   SignalFd.cpp
 * @brief  signalfd 封装的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/SignalFd.h"
#include "afl/log/Log.h"
#include <signal.h>
#include <assert.h>
#include "afl/net/Channel.h"

namespace afl
{
namespace net
{
SignalfdHandler::SignalfdHandler() : m_signalFd(-1)
{
    m_isReady = false;
}

SignalfdHandler::~SignalfdHandler() {}

Signalfd SignalfdHandler::createSignalfd(int flags)
{
    sigemptyset(&m_mask);
    sigaddset(&m_mask, SIGINT); // 创建 signalfd 前须先向掩码加入至少一个信号
    /* 阻塞信号，避免走默认处理方式 */
    if (sigprocmask(SIG_BLOCK, &m_mask, NULL) == -1)
        perror("sigprocmask");

    int sfd = ::signalfd(-1, &m_mask, 0);
    if (sfd < 0)
    {
        LOG_ERROR("signalfd create failure");
        switch (errno)
        {
        case EBADF:  // fd 不是有效文件描述符
        case EINVAL: // flags 无效，或 2.6.26 及更早内核不允许非零 flags
        case EMFILE: // 达到进程可打开 fd 上限
        case ENFILE: // 达到系统可打开文件数上限
        case ENODEV: // 无法挂载内部匿名 inode 设备
        case ENOMEM: // 内存不足，无法创建 signalfd
            break;
        }
    }
    return sfd;
}

void SignalfdHandler::addSigHandler(int sig, const SignalCallback& handler)
{
    m_sigHandlers[sig] = handler;
}

void SignalfdHandler::removeSig(int sig)
{
    SigHandlerMap::iterator iter = m_sigHandlers.find(sig);
    if (iter != m_sigHandlers.end())
        m_sigHandlers.erase(iter);
}

bool SignalfdHandler::haveSignal(int sig)
{
    return m_sigHandlers.find(sig) != m_sigHandlers.end() ? true : false;
}

void SignalfdHandler::registerAll(int flags /* = SFD_NONBLOCK | SFD_CLOEXEC*/)
{
    if (m_signalFd > 0 && m_isReady == true)
        return;

    sigset_t mask;
    sigemptyset(&mask);
    for (SigHandlerMap::iterator iter = m_sigHandlers.begin(); iter != m_sigHandlers.end(); ++iter)
    {
        sigaddset(&mask, iter->first);
    }

    // 阻塞信号，避免走默认处理方式
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        perror("sigprocmask");
    }

    m_signalFd = signalfd(-1, &mask, flags);
    if (m_signalFd < 0)
    {
        LOG_ERROR("signalfd create failure");
        switch (errno)
        {
        case EBADF:  // fd 不是有效文件描述符
        case EINVAL: // flags 无效，或 2.6.26 及更早内核不允许非零 flags
        case EMFILE: // 达到进程可打开 fd 上限
        case ENFILE: // 达到系统可打开文件数上限
        case ENODEV: // 无法挂载内部匿名 inode 设备
        case ENOMEM: // 内存不足，无法创建 signalfd
            break;
        }
    }

    m_isReady = true;
}

int SignalfdHandler::readSig()
{
    struct signalfd_siginfo fdsi;
    ssize_t n = ::read(m_signalFd, &fdsi, sizeof(struct signalfd_siginfo));
    if (n != sizeof(struct signalfd_siginfo))
    {
        LOG_INFO("SignalfdHandler::readSig : read signal error[%d][%d]", n, errno);
    }

    int signo = fdsi.ssi_signo;
    LOG_INFO("SignalfdHandler::readSig : [%d]", signo);
    if (haveSignal(signo))
    {
        if (m_sigHandlers[signo])
            m_sigHandlers[signo](signo);
        else
            LOG_INFO("SignalfdHandler::readSig : signal[%d] have not set callback", signo);
    }
    else
    {
        LOG_INFO("SignalfdHandler::readSig : read unexpected signal[%d]", signo);
    }
    return signo;
}

void SignalfdHandler::wait()
{
    LOG_INFO("SignalfdHandler::wait()");
    while (m_isReady)
    {
        readSig();
    }
}

} // namespace net
} // namespace afl
