/**
 * @file   Semaphore.h
 * @brief  信号量封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/NonCopy.h"
#include <exception>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

int sem_init(sem_t* __sem, int __pshared, unsigned int __value) __attribute__((weak));
int sem_destroy(sem_t* __sem) __attribute__((weak));
int sem_wait(sem_t* __sem) __attribute__((weak));
int sem_post(sem_t* __sem) __attribute__((weak));

namespace afl
{
namespace concurrency
{
/** @brief POSIX 计数信号量封装 */
class Semaphore : public afl::base::NonCopy
{
public:
    explicit Semaphore(int initialcount = 0, int maxcount = 0x7fffffff)
    {
        if (nullptr == sem_init)
        {
            fprintf(stderr, "Using Semaphore, but not used -lpthread link flag!");
            abort();
        }
        sem_init(&m_sem, false, initialcount);
    }

    ~Semaphore() { sem_destroy(&m_sem); }

public:
    bool wait()
    {
        // GDB 下 sem_wait 可能因 EINTR 失败，需重试
        int rc;
        do
        {
            rc = sem_wait(&m_sem);
        } while (rc == -1 && errno == EINTR);
        return rc == 0;
    }

    bool wait(int64_t timeoutMs)
    {
        struct timespec ts;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t usec = tv.tv_usec + timeoutMs * 1000LL;
        ts.tv_sec = tv.tv_sec + usec / 1000000;
        ts.tv_nsec = (usec % 1000000) * 1000;
        return sem_timedwait(&m_sem, &ts) == 0;
    }

    bool tryWait() { return sem_trywait(&m_sem) == 0; }

    bool post(long rc = 1)
    {
        while (rc-- > 0)
        {
            sem_post(&m_sem);
        }
        return true;
    }

private:
    sem_t m_sem;
};

} // namespace concurrency
} // namespace afl
