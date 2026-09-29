/**
 * @file   ThreadPool.h
 * @brief  线程池
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"
#include "afl/concurrency/BlockingQueue.h"

namespace afl
{
namespace concurrency
{
class Thread;

/** @brief 固定数量工作线程 + 任务队列 */
class ThreadPool : afl::base::NonCopy
{
public:
    typedef std::function<void()> Task;

public:
    explicit ThreadPool(const std::string& name = "ThreadPool");
    ~ThreadPool();

public:
    void start(int numThreads);
    void stop();
    void run(const Task& f);
    size_t size() const { return m_queue.size(); }

private:
    void executeThread();

private:
    std::string m_name;
    volatile bool m_running;
    BlockingQueue<Task> m_queue;
    std::vector<Thread*> m_threads;
};

} // namespace concurrency
} // namespace afl
