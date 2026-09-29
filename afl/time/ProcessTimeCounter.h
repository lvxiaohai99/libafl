/**
 * @file   ProcessTimeCounter.h
 * @brief  进程/线程 CPU 时间与资源统计工具
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>

namespace afl
{
namespace time
{
/**
 * @brief 通过 getrusage 统计一段测量期内的内核态/用户态 CPU 时间
 *
 * 使用方式：先调用 start()，再调用 stop()，然后读取各属性。
 * 在完成一次 start()-stop() 周期前读取属性，结果未定义。
 */
class ProcessTimeCounter
{
public:
    typedef int64_t interval_type;

public:
    ProcessTimeCounter();

public:
    /** @brief 开始计时 */
    void start();
    /** @brief 结束计时 */
    void stop();

public:
    /** @brief 内核态耗时（机器相关计数） */
    interval_type kernelPeriodCount() const;
    /** @brief 内核态耗时（整秒） */
    interval_type kernelSeconds() const;
    /** @brief 内核态耗时（整毫秒） */
    interval_type kernelMillSeconds() const;
    /** @brief 内核态耗时（整微秒） */
    interval_type kernelMicroseconds() const;

    /** @brief 用户态耗时（机器相关计数） */
    interval_type userPeriodCount() const;
    /** @brief 用户态耗时（整秒） */
    interval_type userSeconds() const;
    /** @brief 用户态耗时（整毫秒） */
    interval_type userMillSeconds() const;
    /** @brief 用户态耗时（整微秒） */
    interval_type userMicroSeconds() const;

    /** @brief 总耗时（机器相关计数） */
    interval_type periodCount() const;
    /** @brief 总耗时（整秒） */
    interval_type seconds() const;
    /** @brief 总耗时（整毫秒） */
    interval_type millSeconds() const;
    /** @brief 总耗时（整微秒） */
    interval_type microSeconds() const;

private:
    typedef struct timeval timeval_t;
    timeval_t m_kernelStart;
    timeval_t m_kernelEnd;
    timeval_t m_userStart;
    timeval_t m_userEnd;
};

inline ProcessTimeCounter::ProcessTimeCounter()
{
    // 为性能考虑，构造函数不做初始化；须先 start()/stop() 再读属性
}

inline void ProcessTimeCounter::start()
{
    struct rusage r_usage;

    ::getrusage(RUSAGE_SELF, &r_usage);

    m_kernelStart = r_usage.ru_stime;
    m_userStart = r_usage.ru_utime;
}

inline void ProcessTimeCounter::stop()
{
    struct rusage r_usage;

    ::getrusage(RUSAGE_SELF, &r_usage);

    m_kernelEnd = r_usage.ru_stime;
    m_userEnd = r_usage.ru_utime;
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::periodCount() const
{
    return kernelPeriodCount() + userPeriodCount();
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::seconds() const
{
    return kernelSeconds() + userSeconds();
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::millSeconds() const
{
    return kernelMillSeconds() + userMillSeconds();
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::microSeconds() const
{
    return kernelMicroseconds() + userMicroSeconds();
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::kernelPeriodCount() const
{
    return kernelMicroseconds();
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::kernelSeconds() const
{
    return static_cast<interval_type>(m_kernelEnd.tv_sec - m_kernelStart.tv_sec);
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::kernelMillSeconds() const
{
    return kernelMicroseconds() / 1000;
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::kernelMicroseconds() const
{
    return kernelSeconds() * 1000000 +
           static_cast<interval_type>(m_kernelEnd.tv_usec - m_kernelStart.tv_usec);
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::userPeriodCount() const
{
    return userMicroSeconds();
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::userSeconds() const
{
    return static_cast<interval_type>(m_userEnd.tv_sec - m_userStart.tv_sec);
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::userMillSeconds() const
{
    return userMicroSeconds() / 1000;
}

inline ProcessTimeCounter::interval_type ProcessTimeCounter::userMicroSeconds() const
{
    return userSeconds() * 1000000 +
           static_cast<interval_type>(m_userEnd.tv_usec - m_userStart.tv_usec);
}

}  // namespace time
}  // namespace afl
