/**
 * @file   ThreadLocal.h
 * @brief  线程本地存储（TLS）
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include <assert.h>

#include <pthread.h>
namespace afl
{
namespace concurrency
{
/** @brief 基于 pthread_key 的线程局部对象 */
template <typename T>
class ThreadLocal : afl::base::NonCopy
{
public:
    ThreadLocal() { pthread_key_create(&m_tlsKey, &ThreadLocal::cleanHook); }

    ~ThreadLocal()
    {
        T* p = get();
        if (p)
            delete p;
        pthread_key_delete(m_tlsKey);
    }

public:
    T operator()() { return *get(); }

    T& operator*() { return *get(); }

    T* operator->() { return get(); }

    T* get() const
    {
        T* obj = static_cast<T*>(pthread_getspecific(m_tlsKey));
        if (!obj)
        {
            T* newObj = new T;
            pthread_setspecific(m_tlsKey, newObj);
            obj = newObj;
        }
        return obj;
    }

    T* release()
    {
        T* obj = get();
        pthread_setspecific(m_tlsKey, NULL);
        return obj;
    }

    void reset(T* p = 0)
    {
        T* obj = get();
        delete obj;
        obj = 0;
        pthread_setspecific(m_tlsKey, p);
    }

private:
    static void cleanHook(void* x)
    {
        T* obj = static_cast<T*>(x);
        delete obj;
        obj = 0;
    }

private:
    pthread_key_t m_tlsKey;
};

} // namespace concurrency
} // namespace afl
