/**
 * @file   Thread.h
 * @brief  线程封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Mutex.h"
#include <string>
namespace afl
{
namespace concurrency
{
#include <unistd.h>
#include <errno.h>
typedef pthread_t native_thread_handle;

#if !defined(_TTHREAD_CPP11_) && !defined(thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define thread_local __thread
#else
#define thread_local __declspec(thread)
#endif
#endif

/** @brief 基于 pthread 的线程封装 */
class Thread : afl::base::NonCopy
{
public:
    class id;
    typedef std::function<void()> ThreadFunc;

public:
    explicit Thread(const ThreadFunc& func, const std::string& name = "unknown",
                    int policy = SCHED_OTHER, int priority = 0);

    Thread(const ThreadFunc& func, const std::string& name, int policy, int priority, int ssz);
    ~Thread();

public:
    void join();
    void detach();

    bool joinable() const;

    /** @brief 返回线程 id */
    id getId() const;

    native_thread_handle threadHandle() const { return m_threadId; }

    const std::string& threadName() const { return m_threadName; }

    static unsigned int hardwareConcurrency();

private:
    friend struct ThreadImplDataInfo;
    native_thread_handle m_threadId;
    std::string m_threadName;
    bool m_notAThread; ///< 非有效执行线程时为 true
    bool m_joined;     ///< 已调用 join 时为 true
};

/** @brief 线程 ID（参考 tinythread++），唯一标识一条执行线程 @see Thread::getId */
class Thread::id
{
public:
    /** @brief 默认 id 表示尚未绑定执行线程 */
    id() : mId(0) {}

    id(unsigned long int aId) : mId(aId) {}

    id(const id& aId) : mId(aId.mId) {}

    inline id& operator=(const id& aId)
    {
        mId = aId.mId;
        return *this;
    }

    inline friend bool operator==(const id& aId1, const id& aId2) { return (aId1.mId == aId2.mId); }

    inline friend bool operator!=(const id& aId1, const id& aId2) { return (aId1.mId != aId2.mId); }

    inline friend bool operator<=(const id& aId1, const id& aId2) { return (aId1.mId <= aId2.mId); }

    inline friend bool operator<(const id& aId1, const id& aId2) { return (aId1.mId < aId2.mId); }

    inline friend bool operator>=(const id& aId1, const id& aId2) { return (aId1.mId >= aId2.mId); }

    inline friend bool operator>(const id& aId1, const id& aId2) { return (aId1.mId > aId2.mId); }

    inline friend std::ostream& operator<<(std::ostream& os, const id& obj)
    {
        os << obj.mId;
        return os;
    }

    unsigned long int value() const { return mId; }

private:
    unsigned long int mId;
};


// 精简 ratio，供 chrono 使用
typedef long long __intmax_t;

/** @brief 比例模板（精简版 std::ratio） */
template <__intmax_t N, __intmax_t D = 1>
class ratio
{
public:
    static double _as_double() { return double(N) / double(D); }
};

/** @brief 精简 chrono 时间间隔类型 */
namespace chrono
{
/** @brief 时间长度，供 this_thread::sleepFor 使用 */
template <class _Rep, class _Period = ratio<1>>
class duration
{
private:
    _Rep m_rep;

public:
    typedef _Rep rep;
    typedef _Period period;

    /** @brief 以 rep 构造时长 */
    template <class _Rep2>
    explicit duration(const _Rep2& r) : m_rep(r)
    {
    }

    /** @brief 返回时长数值 */
    rep count() const { return m_rep; }
};

typedef duration<__intmax_t, ratio<1, 1000000000>> nanoseconds;   ///< 纳秒
typedef duration<__intmax_t, ratio<1, 1000000>> microseconds;     ///< 微秒
typedef duration<__intmax_t, ratio<1, 1000>> milliseconds;        ///< 毫秒
typedef duration<__intmax_t> seconds;                             ///< 秒
typedef duration<__intmax_t, ratio<60>> minutes;                  ///< 分钟
typedef duration<__intmax_t, ratio<3600>> hours;                  ///< 小时
} // namespace chrono

/** @brief 当前线程相关操作 */
namespace this_thread
{
/** @brief 当前线程 id */
Thread::id getId();

extern thread_local int g_currentTid;
void cacheThreadTid();

/** @brief 内核线程 id（TID） */
inline int tid()
{
    if (g_currentTid == 0)
        cacheThreadTid();

    return g_currentTid;
}

/** @brief 让出 CPU，供调度器切换线程 */
inline void yield()
{
    sched_yield();
}

/**
 * @brief 阻塞当前线程指定时长
 * @param aTime 最小时长
 * @note 支持 nanoseconds、microseconds、milliseconds、seconds、minutes、hours
 */
template <class _Rep, class _Period>
void sleepFor(const chrono::duration<_Rep, _Period>& aTime)
{
    while (usleep(int(double(aTime.count()) * (1000000.0 * _Period::_as_double()) + 0.5)) != 0 &&
           errno == EINTR)
        ;
}

/** @brief 休眠 millsecond 毫秒 */
inline void sleep(uint32_t millsecond)
{
    sleepFor(chrono::milliseconds(millsecond));
}
} // namespace this_thread

} // namespace concurrency
} // namespace afl
