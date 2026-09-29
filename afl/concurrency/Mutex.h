/**
 * @file   Mutex.h
 * @brief  互斥锁封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

#include <unistd.h>
#include <pthread.h>
#include <errno.h>

namespace afl
{
namespace concurrency
{
#ifdef NDEBUG

#define THREAD_CHECK(func)                                                                         \
    do                                                                                             \
    {                                                                                              \
        int errnum = (func);                                                                       \
        if (errnum != 0)                                                                           \
            fprintf(stderr, "%s:%d : [%d]\n", __FILE__, __LINE__, errnum);                         \
    } while (0)

#else

#define THREAD_CHECK(func)                                                                         \
    do                                                                                             \
    {                                                                                              \
        int errnum = (func);                                                                       \
        if (errnum != 0)                                                                           \
        {                                                                                          \
            fprintf(stderr, "%s:%d : [%d]\n", __FILE__, __LINE__, errnum);                         \
            assert(errnum == 0);                                                                   \
        }                                                                                          \
    } while (0)

#endif

class NullMutex
{
    DISALLOW_COPY_AND_ASSIGN(NullMutex);

public:
    NullMutex() {}
    ~NullMutex() {}

public:
    void lock() {}

    bool tryLock() { return true; }

    void unlock() {}
};

class SpinMutex
{
public:
    SpinMutex() { m_lock = 0; }

    ~SpinMutex() {}

    void lock()
    {
        while (__sync_lock_test_and_set(&m_lock, 1)) {}
    }

    void unlock() { __sync_lock_release(&m_lock); }

private:
    volatile int m_lock;
};

/** @brief pthread 互斥锁 RAII 封装 */
class Mutex
{
    DISALLOW_COPY_AND_ASSIGN(Mutex);

public:
    Mutex() { pthread_mutex_init(&m_mutex, NULL); }

    ~Mutex() { pthread_mutex_destroy(&m_mutex); }

public:
    void lock() { THREAD_CHECK(pthread_mutex_lock(&m_mutex)); }

    bool tryLock()
    {
#if (_WIN32_WINNT >= 0x0400)
#else
#endif
        return pthread_mutex_trylock(&m_mutex) == 0;
    }

    void unlock() { THREAD_CHECK(pthread_mutex_unlock(&m_mutex)); }

    pthread_mutex_t* getMutex() { return &m_mutex; }

private:
    pthread_mutex_t m_mutex;
};


class RecursiveMutex
{
    DISALLOW_COPY_AND_ASSIGN(RecursiveMutex);

public:
    RecursiveMutex()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
    }

    ~RecursiveMutex() { pthread_mutex_destroy(&m_mutex); }

    void lock() { THREAD_CHECK(pthread_mutex_lock(&m_mutex)); }

    bool tryLock() { return pthread_mutex_trylock(&m_mutex) == 0; }

    void unlock() { THREAD_CHECK(pthread_mutex_unlock(&m_mutex)); }

    pthread_mutex_t* getMutex() { return &m_mutex; }

private:
    pthread_mutex_t m_mutex;
};

template <class MutexType>
class LockGuard
{
    DISALLOW_COPY_AND_ASSIGN(LockGuard);

public:
    explicit LockGuard(MutexType& mutex) : m_mutex(mutex) { m_mutex.lock(); }
    ~LockGuard() { m_mutex.unlock(); }

private:
    MutexType& m_mutex;
};

template <class MutexType>
class MutexTryLockGuard
{
    DISALLOW_COPY_AND_ASSIGN(MutexTryLockGuard);

public:
    explicit MutexTryLockGuard(MutexType& mutex) : m_mutex(mutex)
    {
        m_isLocked = m_mutex.tryLock();
    }
    ~MutexTryLockGuard()
    {
        if (m_isLocked)
            m_mutex.unlock();
    }
    bool IsLocked() { return m_isLocked; }

private:
    bool m_isLocked;
    MutexType& m_mutex;
};

} // namespace concurrency
} // namespace afl
