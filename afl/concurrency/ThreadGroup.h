/**
 * @file   ThreadGroup.h
 * @brief  线程组管理
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/Thread.h"
#include "afl/concurrency/Mutex.h"

namespace afl
{
namespace concurrency
{
class Thread;

/** @brief 管理一组 Thread 的生命周期与 join */
class ThreadGroup : afl::base::NonCopy
{
public:
    ThreadGroup();
    ~ThreadGroup();

public:
    template <typename F>
    Thread* createThread(F threadfunc, const std::string& thrd_name = "")
    {
        Thread* trd = new Thread(threadfunc, thrd_name);
        addThread(trd);
        return trd;
    }

    void addThread(Thread* thd);
    void removeThread(Thread* thd);
    void joinAll();
    size_t size() const;

private:
    bool isThisThreadIn();
    bool is_thread_in(Thread* thrd);

private:
    mutable Mutex m_mutex;
    std::vector<Thread*> m_threads;
};

} // namespace concurrency
} // namespace afl
