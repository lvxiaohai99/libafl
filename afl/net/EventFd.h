/**
 * @file   EventFd.h
 * @brief  eventfd 封装（进程及线程间的事件通知）；需 Linux 内核 ≥ 2.6.22
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

#include <stdint.h>
#include <sys/eventfd.h>

namespace afl
{
namespace net
{
/** eventfd 读写单位 sizeof(uint64_t)（8 字节），为 64 位计数器；计数非零表示可读 */
/** 写操作递增计数；读操作取回计数并清零 */

class EventfdHandler
{
public:
    EventfdHandler(unsigned int initval = 0, int flags = EFD_NONBLOCK | EFD_CLOEXEC);
    ~EventfdHandler();

public:
    int fd() { return m_eventfd; }

    void notify() { write(1); }

    ssize_t write(uint64_t value = 1);

    ssize_t read(uint64_t* value = NULL);

private:
    /** @brief 创建 eventfd；成功返回 fd，失败返回 -1 */
    int createEventfd(unsigned int initval, int flags);

private:
    int m_eventfd;
};

} // namespace net
} // namespace afl
