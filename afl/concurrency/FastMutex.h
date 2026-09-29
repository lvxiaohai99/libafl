/**
 * @file   FastMutex.h
 * @brief  快速互斥锁（参考 tinythreadpp）
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

#include <unistd.h>
#include <pthread.h>
#include <sched.h>

// 若平台支持则使用汇编实现，否则回退到 pthread 系统 API
#if (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))) ||                           \
    (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))) ||                                \
    (defined(__GNUC__) && (defined(__ppc__)))
#define FAST_MUTEX_ASM
#endif

namespace afl
{
namespace concurrency
{
class FastMutex
{
    DISALLOW_COPY_AND_ASSIGN(FastMutex);

public:
    FastMutex()
    {
#if defined(FAST_MUTEX_ASM)
        m_lock = 0;
#elif defined(OS_WINDOWS)
        InitializeCriticalSection(&m_mutex);
#elif defined(OS_LINUX)
        pthread_mutex_init(&m_mutex, NULL);
#endif
    }

    ~FastMutex()
    {
#if defined(FAST_MUTEX_ASM)
#elif defined(OS_WINDOWS)
        DeleteCriticalSection(&m_mutex);
#elif defined(OS_LINUX)
        pthread_mutex_destroy(&m_mutex);
#endif
    }

public:
    void lock()
    {
#if defined(FAST_MUTEX_ASM)
        bool gotLock;
        do
        {
            gotLock = tryLock();
            if (!gotLock)
            {
                sched_yield();
            }
        } while (!gotLock);
#elif defined(OS_WINDOWS)
        EnterCriticalSection(&m_mutex);
#elif defined(OS_LINUX)
        pthread_mutex_lock(&m_mutex);
#endif
    }

    bool tryLock()
    {
#if defined(FAST_MUTEX_ASM)
        int oldLock;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
        asm volatile("movl $1,%%eax\n\t"
                     "xchg %%eax,%0\n\t"
                     "movl %%eax,%1\n\t"
                     : "=m"(m_lock), "=m"(oldLock)
                     :
                     : "%eax", "memory");
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        int* ptrLock = &m_lock;
        __asm
        {
            mov eax, 1
            mov ecx, ptrLock
            xchg eax, [ecx]
            mov oldLock, eax
        }
#elif defined(__GNUC__) && (defined(__ppc__))
        int newLock = 1;
        asm volatile("\n1:\n\t"
                     "lwarx  %0,0,%1\n\t"
                     "cmpwi  0,%0,0\n\t"
                     "bne-   2f\n\t"
                     "stwcx. %2,0,%1\n\t"
                     "bne-   1b\n\t"
                     "isync\n"
                     "2:\n\t"
                     : "=&r"(oldLock)
                     : "r"(&m_lock), "r"(newLock)
                     : "cr0", "memory");
#endif
        return (oldLock == 0);
#elif defined(OS_WINDOWS)
        return TryEnterCriticalSection(&m_mutex) ? true : false;
#elif defined(OS_LINUX)
        return (pthread_mutex_trylock(&m_mutex) == 0);
#endif
    }

    void unlock()
    {
#if defined(FAST_MUTEX_ASM)
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
        asm volatile("movl $0,%%eax\n\t"
                     "xchg %%eax,%0\n\t"
                     : "=m"(m_lock)
                     :
                     : "%eax", "memory");
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        int* ptrLock = &m_lock;
        __asm
        {
            mov eax, 0
            mov ecx, ptrLock
            xchg eax, [ecx]
        }
#elif defined(__GNUC__) && (defined(__ppc__))
        asm volatile("sync\n\t" // PowerPC：可用 lwsync 替代 sync（若平台允许）
                     :
                     :
                     : "memory");
        m_lock = 0;
#endif
#elif defined(OS_WINDOWS)
        LeaveCriticalSection(&m_mutex);
#elif defined(OS_LINUX)
        pthread_mutex_unlock(&m_mutex);
#endif
    }

#if defined(FAST_MUTEX_ASM)

#elif defined(OS_WINDOWS)
    CRITICAL_SECTION* getMutex() { return &m_mutex; }
#elif defined(OS_LINUX)
    pthread_mutex_t* getMutex() { return &m_mutex; }
#endif

private:
#if defined(FAST_MUTEX_ASM)
    int m_lock;
#elif defined(OS_WINDOWS)
    mutable CRITICAL_SECTION m_mutex;
#elif defined(OS_LINUX)
    pthread_mutex_t m_mutex;
#endif
};

} // namespace concurrency
} // namespace afl
