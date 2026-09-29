/**
 * @file   EventFd.cpp
 * @brief  eventfd 封装的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/SocketUtil.h"
#include "afl/net/EventFd.h"
#include "afl/log/Log.h"
#include <sys/eventfd.h>
#include <unistd.h>

namespace afl
{
namespace net
{
EventfdHandler::EventfdHandler(unsigned int initval /* = 0 */,
                               int flags /* = EFD_NONBLOCK | EFD_CLOEXEC */)
{
    m_eventfd = -1;
    createEventfd(initval, flags);
}

EventfdHandler::~EventfdHandler()
{
    if (m_eventfd != -1)
    {
        ::close(m_eventfd);
        m_eventfd = -1;
    }
}

int EventfdHandler::createEventfd(unsigned int initval, int flags)
{
    int efd = ::eventfd(initval, flags);
    if (efd < 0)
    {
        LOG_ALERT("create eventfd failed in EventfdHandler::createEventfd()");
        return efd;
    }
    LOG_DEBUG("EventfdHandler::createEventfd [%d]", efd);

    m_eventfd = efd;
    return efd;
}

ssize_t EventfdHandler::write(uint64_t value /* = 1 */)
{
    ssize_t n = ::write(m_eventfd, &value, sizeof(value));
    if (n != sizeof(value)) // just write one uint64_t
    {
        LOG_ERROR("EventfdHandler::write(): write error[%d][%d][%d]", m_eventfd, n, errno);
    }
    return n;
}

ssize_t EventfdHandler::read(uint64_t* value /* = NULL*/)
{
    ssize_t n;
    if (value == NULL)
    {
        uint64_t tmp;
        n = ::read(m_eventfd, &tmp, sizeof(uint64_t));
    }
    else
    {
        n = ::read(m_eventfd, value, sizeof(uint64_t));
    }

    if (n != sizeof(uint64_t)) //always return 8 byte
    {
        LOG_ERROR("EventfdHandler::read(): read error[%d][%d][%d][%s]", m_eventfd, n, errno,
                  strerror(errno));
    }

    return n;
}

} // namespace net
} // namespace afl
