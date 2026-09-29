/**
 * @file   Thread.cpp
 * @brief  线程封装的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/concurrency/Thread.h"
#include "afl/base/Exception.h"

#include <syscall.h>

int pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)
    __attribute__((weak));

int pthread_attr_setstacksize(pthread_attr_t* __attr, size_t __stacksize) __attribute__((weak));

int pthread_join(pthread_t __th, void** __thread_return) __attribute__((weak));

int pthread_detach(pthread_t __th) __attribute__((weak));

namespace afl
{
namespace concurrency
{
//#define DO_NOT_USE_TRY_CATCH
namespace detail
{
struct ThreadImplDataInfo
{
    typedef afl::concurrency::Thread::ThreadFunc ThreadFunc;
    ThreadFunc m_func;
    std::string m_name;

    ThreadImplDataInfo(const ThreadFunc& func, const std::string& threadName)
        : m_func(func), m_name(threadName)
    {
    }

    void runThread()
    {
#ifdef DO_NOT_USE_TRY_CATCH
        m_func();
#else
        try
        {
            m_func();
        }
        catch (const afl::base::Exception& ex)
        {
            fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            std::abort();
        }
        catch (const std::exception& ex)
        {
            fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            std::abort();
        }
        catch (...)
        {
            fprintf(stderr, "uncaught exception caught in Thread %s\n", m_name.c_str());
            // 未捕获异常将终止进程（C++11 默认行为）
            std::terminate();
        }
#endif
    }
};

void* startThread(void* arg)
{
    ThreadImplDataInfo* data = static_cast<ThreadImplDataInfo*>(arg);
    data->runThread();
    delete data;
    return 0;
}
} // namespace detail

Thread::Thread(const ThreadFunc& func, const std::string& name /* = unknown*/,
               int policy /*= SCHED_OTHER*/, int priority /*= 0*/)
    : m_threadId(0)
      //, m_threadFunc(func)
      ,
      m_threadName(name), m_notAThread(true), m_joined(false)
{
    detail::ThreadImplDataInfo* data = new detail::ThreadImplDataInfo(func, name);
    // 创建 pthread

    if (nullptr == pthread_create)
    {
        fprintf(stderr, "Using afl::concurrency::thread, but not used -lpthread link flag!");
        abort();
    }

    struct sched_param param;
    pthread_attr_t pAttr;
    memset(&pAttr, 0x00, sizeof(pAttr));
    pthread_attr_init(&pAttr);
    pthread_attr_setscope(&pAttr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setstacksize(&pAttr, 128 * 1024);

    if (policy == SCHED_FIFO || policy == SCHED_RR)
    {
        pthread_attr_setschedpolicy(&pAttr, policy);
        pthread_attr_getschedparam(&pAttr, &param);

        priority = ((priority > sched_get_priority_max(policy)) || (priority < 0)) ? 0 : priority;
        param.sched_priority = (0 == geteuid()) ? (priority) : 0;
        pthread_attr_setschedparam(&pAttr, &param);
    }

    if (pthread_create(&m_threadId, &pAttr, detail::startThread, data) != 0)
        m_threadId = 0;

    if (!m_threadId)
    {
        delete data;
        std::abort();
    }
    m_notAThread = false; // 线程已创建
}

Thread::Thread(const ThreadFunc& func, const std::string& name /* = unknown */,
               int policy /*= SCHED_OTHER*/, int priority /*= 0*/, int ssz)
    : m_threadId(0)
      //, m_threadFunc(func)
      ,
      m_threadName(name), m_notAThread(true), m_joined(false)
{
    detail::ThreadImplDataInfo* data = new detail::ThreadImplDataInfo(func, name);
    // 创建 pthread

    if (nullptr == pthread_create)
    {
        fprintf(stderr, "Using afl::concurrency::thread, but not used -lpthread link flag!");
        abort();
    }

    struct sched_param param;
    pthread_attr_t pAttr;
    memset(&pAttr, 0x00, sizeof(pAttr));
    pthread_attr_init(&pAttr);
    pthread_attr_setscope(&pAttr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setstacksize(&pAttr, ssz);

    if (policy == SCHED_FIFO || policy == SCHED_RR)
    {
        pthread_attr_setschedpolicy(&pAttr, policy);
        pthread_attr_getschedparam(&pAttr, &param);

        priority = ((priority > sched_get_priority_max(policy)) || (priority < 0)) ? 0 : priority;
        param.sched_priority = (0 == geteuid()) ? (priority) : 0;
        pthread_attr_setschedparam(&pAttr, &param);
    }

    if (pthread_create(&m_threadId, &pAttr, detail::startThread, data) != 0)
        m_threadId = 0;

    if (!m_threadId)
    {
        delete data;
        std::abort();
    }
    m_notAThread = false; // 线程已创建
}

Thread::~Thread()
{
    if (joinable())
        std::terminate();
}

bool Thread::joinable() const
{
    return !m_joined && !m_notAThread;
}

void Thread::join()
{
    if (joinable())
    {
        pthread_join(m_threadId, NULL);
        m_joined = true;
    }
}

void Thread::detach()
{
    // 未使用 joinable()；重复 detach 会导致 terminate
    if (!m_joined /*joinable()*/)
    {
        pthread_detach(m_threadId);
        m_notAThread = true;
    }
}

Thread::id Thread::getId() const
{
    if (!joinable())
        return id();
    return id(m_threadId);
}

/*static*/ unsigned int Thread::hardwareConcurrency()
{
#if defined(_SC_NPROCESSORS_ONLN)
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    return (int)sysconf(_SC_NPROC_ONLN);
#else
    // 标准规定无法探测硬件核数时返回 0
    return 0;
#endif
}

//------------------------------------------------------------------------------
// this_thread 命名空间实现
//------------------------------------------------------------------------------
namespace this_thread
{
thread_local int g_currentTid = 0;

void cacheThreadTid()
{
    g_currentTid = static_cast<int>(::syscall(SYS_gettid));
}

int gettid()
{
    if (g_currentTid == 0)
    {
        cacheThreadTid();
    }
    return g_currentTid;
}

Thread::id getId()
{
    return Thread::id(pthread_self());
}
} // namespace this_thread

} // namespace concurrency
} // namespace afl
