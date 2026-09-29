/**
 * @file   BlockingQueueWorker.h
 * @brief  阻塞队列工作调度器，工作于 BlockingQueue 或 BoundedBlockingQueue 之上
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/concurrency/BlockingQueue.h"
#include "afl/concurrency/ThreadGroup.h"


namespace afl
{
namespace concurrency
{
/** @brief 从阻塞队列取任务并在工作线程中执行 */
template <typename Queue>
class BlockingQueueWorker
{
public:
    typedef Queue QueueType;
    typedef typename Queue::JobType JobType;
    typedef std::function<void(JobType&)> FunctionType;

    template <typename FunctionType>
    BlockingQueueWorker(QueueType& queue, const FunctionType& function, int thread_num = 1)
        : m_queue(queue), m_function(function), m_threadNum(thread_num)
    {
    }

    BlockingQueueWorker(QueueType& queue, int thread_num = 1)
        : m_queue(queue), m_function(NULL), m_threadNum(thread_num)
    {
    }

    ~BlockingQueueWorker() { stop(); }

    void start()
    {
        if (m_threads.size() > 0)
            return;
        for (int i = 0; i < m_threadNum; ++i)
        {
            m_threads.createThread(std::bind(&BlockingQueueWorker::doWork, this));
        }
    }

    template <typename FunctionType>
    void start(const FunctionType& function)
    {
        m_function = function;
        start();
    }

    void stop()
    {
        m_function = 0;
        m_queue.stop();
        m_threads.joinAll();
    }

private:
    void doWork()
    {
        for (;;)
        {
            JobType job;
            bool bret = m_queue.pop(job);
            if (!bret)
                break;
            if (m_function)
            {
                m_function(job);
            }
        }
    }

private:
    QueueType& m_queue;
    FunctionType m_function;
    int m_threadNum;
    ThreadGroup m_threads;
};

} // namespace concurrency
} // namespace afl
