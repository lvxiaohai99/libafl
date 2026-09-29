/**
 * @file   RWMutex.h
 * @brief  读写锁封装
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

namespace afl
{
namespace concurrency
{
/**
 * @brief pthread 读写锁封装
 */
class RWMutex
{
    DISALLOW_COPY_AND_ASSIGN(RWMutex);

public:
    RWMutex() { pthread_rwlock_init(&m_rwlock, NULL); }

    ~RWMutex() { pthread_rwlock_destroy(&m_rwlock); }

    /** @brief 加读锁 @return 是否成功 */
    bool readLock() { return pthread_rwlock_rdlock(&m_rwlock) == 0; }

    /** @brief 解读锁 @return 是否成功 */
    bool readUnLock() { return pthread_rwlock_unlock(&m_rwlock) == 0; }

    /** @brief 加写锁 @return 是否成功 */
    bool writeLock() { return pthread_rwlock_wrlock(&m_rwlock) == 0; }

    /** @brief 解写锁 @return 是否成功 */
    bool writeUnLock() { return pthread_rwlock_unlock(&m_rwlock) == 0; }

    /** @brief 尝试读锁 @return 是否成功 */
    bool tryReadLock() { return pthread_rwlock_tryrdlock(&m_rwlock) == 0; }

    /** @brief 尝试写锁 @return 是否成功 */
    bool tryWriteLock() { return pthread_rwlock_trywrlock(&m_rwlock) == 0; }

private:
    pthread_rwlock_t m_rwlock;
};

/**
 * @brief 读锁 RAII 守卫
 * @tparam LockType 读写锁类型（通常为 RWMutex）
 */
template <class LockType>
class RWMutexReadLockGuard
{
    DISALLOW_COPY_AND_ASSIGN(RWMutexReadLockGuard);

public:
    /** @param mutex 读写锁 */
    explicit RWMutexReadLockGuard(LockType& mutex) : m_mutex(mutex) { m_mutex.readLock(); }
    ~RWMutexReadLockGuard() { m_mutex.readUnLock(); }

private:
    LockType& m_mutex;
};

/**
 * @brief 写锁 RAII 守卫
 * @tparam LockType 读写锁类型（通常为 RWMutex）
 */
template <class LockType>
class RWMutexWriteLockGuard
{
    DISALLOW_COPY_AND_ASSIGN(RWMutexWriteLockGuard);

public:
    /** @param mutex 读写锁 */
    explicit RWMutexWriteLockGuard(LockType& mutex) : m_mutex(mutex) { m_mutex.writeLock(); }
    ~RWMutexWriteLockGuard() { m_mutex.writeUnLock(); }

private:
    LockType& m_mutex;
};

} // namespace concurrency
} // namespace afl
