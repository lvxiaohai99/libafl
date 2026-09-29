/**
 * @file   Atomic.h
 * @brief  原子操作计数器
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

#include <pthread.h>
#define ATOMIC_ADD(ptr, v) __sync_add_and_fetch(ptr, v)
#define ATOMIC_SUB(ptr, v) __sync_sub_and_fetch(ptr, v)
#define ATOMIC_ADD_AND_FETCH(ptr, v) __sync_add_and_fetch(ptr, v)
#define ATOMIC_SUB_AND_FETCH(ptr, v) __sync_sub_and_fetch(ptr, v)
#define ATOMIC_FETCH_AND_ADD(ptr, v) __sync_fetch_and_add(ptr, v)
#define ATOMIC_FETCH_AND_SUB(ptr, v) __sync_fetch_and_sub(ptr, v)
#define ATOMIC_FETCH(ptr) __sync_add_and_fetch(ptr, 0)
#define ATOMIC_SET(ptr, v) __sync_val_compare_and_swap(ptr, *(ptr), v)
#define ATOMIC_CAS(ptr, cmp, v) __sync_bool_compare_and_swap(ptr, cmp, v)


namespace afl
{
namespace concurrency
{
/** @brief GCC 内置原子整型封装 */
template <typename T>
class Atomic
{
public:
    Atomic()
    {
        memset((void*)&m_atomic, 0x00, sizeof(m_atomic));
        ATOMIC_SET(&m_atomic, 0);
    }

public:
    void add(T n = 1) { ATOMIC_ADD(&m_atomic, n); }

    T getAndAdd(T n) { return ATOMIC_FETCH_AND_ADD(&m_atomic, n); }

    T addAndGet(T n)
    {
        return ATOMIC_ADD_AND_FETCH(&m_atomic, n);
    }

    void sub(T n = 1) { ATOMIC_SUB(&m_atomic, n); }

    T getAndSub(T n) { return ATOMIC_FETCH_AND_SUB(&m_atomic, n); }

    T subAndGet(T n)
    {
        return ATOMIC_SUB_AND_FETCH(&m_atomic, n);
    }

    T increment() { return addAndGet(1); }

    T decrement() { return addAndGet(-1); }

    T value() { return ATOMIC_FETCH(&m_atomic); }

public:
    T operator++() { return addAndGet(1); }

    T operator--() { return subAndGet(1); }

    T operator++(int) { return getAndAdd(1); }

    T operator--(int) { return getAndSub(1); }

    T operator+=(T n) { return addAndGet(n); }

    T operator-=(T n) { return subAndGet(n); }

    void operator=(T n) { ATOMIC_SET(&m_atomic, n); }
    bool operator==(T n) { return (value() == n); }

    operator T() { return value(); }

private:
    volatile T m_atomic;
};

template <>
class Atomic<bool>
{
public:
    Atomic()
    {
        memset((void*)&m_atomic, 0x00, sizeof(m_atomic));
        ATOMIC_SET(&m_atomic, 0);
    }

    Atomic(bool value)
    {
        memset((void*)&m_atomic, 0x00, sizeof(m_atomic));
        ATOMIC_SET(&m_atomic, value ? 1 : 0);
    }

    Atomic& operator=(bool value)
    {
        ATOMIC_SET(&m_atomic, value ? 1 : 0);
        return *this;
    }

    /** @brief 置为 false */
    bool clear()
    {
        return ATOMIC_SET(&m_atomic, 0);
    }

    /** @brief 置 true 并返回旧值 */
    bool testAndSet()
    {
        return ATOMIC_SET(&m_atomic, 1);
    }

    bool value() { return ATOMIC_FETCH(&m_atomic); }

    operator bool() { return ATOMIC_FETCH(&m_atomic); }

private:
    volatile int m_atomic;
};


typedef Atomic<int32_t> AtomicInt32;
typedef Atomic<int64_t> AtomicInt64;
typedef Atomic<bool> AtomicBool;

} // namespace concurrency
} // namespace afl
