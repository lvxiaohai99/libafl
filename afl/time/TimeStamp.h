/**
 * @file   TimeStamp.h
 * @brief  时间戳封装
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <string>

#define AFL_MSEC_PER_SEC (1000)
#define AFL_USEC_PER_SEC (1000 * 1000)
#define AFL_TIME_SEC(time) ((time) / AFL_USEC_PER_SEC)

namespace afl
{
namespace time
{
class TimeStamp
{
public:
    TimeStamp();
    explicit TimeStamp(int64_t ms);

public:
    /**
     * @brief 获取当前时间戳
     * @param isSystemTime true 为系统时间，false 为自进程启动起的单调时间（默认）
     */
    static TimeStamp now(bool isSystemTime = false);
    static TimeStamp invalid();

    /** @brief 两时间戳相差的秒数（浮点） */
    static double timeDiffS(const TimeStamp& end, const TimeStamp& start)
    {
        int64_t delta = end.microSeconds() - start.microSeconds();
        return AFL_TIME_SEC(delta * 1.0);
    }

    /** @brief 两时间戳相差的毫秒数 */
    static int64_t timeDiffMS(const TimeStamp& end, const TimeStamp& start)
    {
        int64_t delta = end.microSeconds() - start.microSeconds();
        return (delta / AFL_MSEC_PER_SEC);
    }

    /** @brief 两时间戳相差的微秒数 */
    static int64_t timeDiffUS(const TimeStamp& end, const TimeStamp& start)
    {
        return end.microSeconds() - start.microSeconds();
    }

public:
    int64_t microSeconds() const { return m_microSeconds; }

    int64_t millSeconds() const { return m_microSeconds / AFL_MSEC_PER_SEC; }

    int64_t seconds() const { return m_microSeconds / AFL_USEC_PER_SEC; }

    bool valid() const { return m_microSeconds > 0; }

    void swap(TimeStamp& that) { std::swap(m_microSeconds, that.m_microSeconds); }

    struct tm getTm(bool showlocaltime = true) const;
    std::string toString(bool showlocaltime = true) const;

private:
    int64_t m_microSeconds;
};


inline std::ostream& operator<<(std::ostream& out, const TimeStamp& ts)
{
    out << ts.toString();
    return out;
}

inline bool operator<(const TimeStamp& lhs, const TimeStamp& rhs)
{
    return lhs.microSeconds() < rhs.microSeconds();
}

inline bool operator==(const TimeStamp& lhs, const TimeStamp& rhs)
{
    return lhs.microSeconds() == rhs.microSeconds();
}

/** @brief 时间戳相减，结果为微秒差 */
inline int64_t operator-(const TimeStamp& end, const TimeStamp& start)
{
    return end.microSeconds() - start.microSeconds();
}

inline TimeStamp operator+(const TimeStamp& lhs, double seconds)
{
    int64_t delta = seconds * AFL_USEC_PER_SEC;
    return TimeStamp(lhs.microSeconds() + delta);
}

inline TimeStamp operator+(double seconds, const TimeStamp& rhs)
{
    return (rhs + seconds);
}

inline TimeStamp operator+=(TimeStamp& lhs, double seconds)
{
    return lhs = lhs + seconds;
}

inline TimeStamp operator-(const TimeStamp& lhs, double seconds)
{
    return (lhs + (-seconds));
}

inline TimeStamp operator-=(TimeStamp& lhs, double seconds)
{
    return (lhs = (lhs + (-seconds)));
}

} // namespace time
} // namespace afl
