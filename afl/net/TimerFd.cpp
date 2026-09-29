/**
 * @file   TimerFd.cpp
 * @brief  timerfd 封装的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/log/Log.h"
#include "afl/net/TimerFd.h"
#include "afl/net/SocketUtil.h"

namespace afl
{
namespace net
{
TimerfdHandler::TimerfdHandler(int clockid /* = CLOCK_MONOTONIC*/,
                               int flags /* = TFD_NONBLOCK | TFD_CLOEXEC*/)
    : m_timerfd(-1)
{
    m_timerfd = ::timerfd_create(clockid, flags); //创建个定时器描述
    if (m_timerfd < 0)
    {
        LOG_ERROR("TimerfdHandler create timerfd failure [%d] [%s]", errno, strerror(errno));
    }
}

TimerfdHandler::~TimerfdHandler()
{
    if (m_timerfd > 0)
    {
        ::close(m_timerfd);
    }
}

static struct timespec howMuchTimeFromNow(TimeStamp when)
{
    int64_t microseconds = when.microSeconds() - TimeStamp::now().microSeconds();
    if (microseconds < 100)
    {
        microseconds = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / AFL_USEC_PER_SEC);
    ts.tv_nsec = static_cast<long>((microseconds % AFL_USEC_PER_SEC) * 1000);
    return ts;
}

void TimerfdHandler::resetTimerfd(TimeStamp expiration, int interval_us /* = 0*/)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));

    newValue.it_value = howMuchTimeFromNow(expiration);
    if (interval_us > 0)
    {
        newValue.it_interval.tv_sec = interval_us / 1000000;
        newValue.it_interval.tv_nsec = interval_us % 1000000;
    }

    int ret = ::timerfd_settime(m_timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_ERROR("resetTimerfd [%d] [%d] [%s]", m_timerfd, ret, strerror(errno));
    }
}

//设置新的超时时间
void TimerfdHandler::resetTimerfd(uint64_t next_expire_us, int interval_us /* = 0*/)
{
    TimeStamp expiration(TimeStamp::now().microSeconds() + next_expire_us);
    resetTimerfd(expiration, interval_us);
}

//从时间文件描述符获得当前有多少个定时器超
uint64_t TimerfdHandler::read(uint64_t* howmany)
{
    ssize_t n = ::read(m_timerfd, howmany, sizeof(uint64_t));
    LOG_DEBUG("readTimerfd [%d] [%d]", m_timerfd, *howmany);
    if (n != sizeof(uint64_t))
    {
        LOG_ERROR("readTimerfd [%d] [%d] [%s]", m_timerfd, *howmany);
    }
    return n;
}

void TimerfdHandler::stop()
{
    struct itimerspec newValue;
    bzero(&newValue, sizeof(newValue));
    ::timerfd_settime(m_timerfd, 0, &newValue, NULL);
}

} // namespace net
} // namespace afl
