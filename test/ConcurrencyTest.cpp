/**
 * @file   ConcurrencyTest.cpp
 * @brief  concurrency 模块单元测试：BlockingQueue / BoundedBlockingQueue /
 *         ThreadPool / CountDownLatch / ConcurrentQueue
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>

#include "afl/concurrency/BlockingQueue.h"
#include "afl/concurrency/BoundedBlockingQueue.h"
#include "afl/concurrency/ThreadPool.h"
#include "afl/concurrency/CountDownLatch.h"
#include "afl/concurrency/ConcurrentQueue.h"

using namespace afl::concurrency;

// ---------------------------------------------------------------------------
// BlockingQueue
// ---------------------------------------------------------------------------
TEST(ConcurrencyTest, BlockingQueueSingleThread)
{
    BlockingQueue<int> queue;
    EXPECT_TRUE(queue.empty());

    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_EQ(2u, queue.size());

    int v = 0;
    EXPECT_TRUE(queue.pop(v));
    EXPECT_EQ(1, v);
    EXPECT_TRUE(queue.pop(v));
    EXPECT_EQ(2, v);
    EXPECT_TRUE(queue.empty());
}

TEST(ConcurrencyTest, BlockingQueueMultiThread)
{
    BlockingQueue<int> queue;
    std::atomic<int> consumed(0);
    const int kItems = 1000;

    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c)
    {
        consumers.emplace_back([&] {
            int v = 0;
            while (queue.pop(v))
            {
                consumed.fetch_add(1);
            }
        });
    }

    for (int i = 0; i < kItems; ++i)
        queue.push(i);
    while (consumed.load() < kItems)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    queue.stop();
    for (auto& t : consumers)
        t.join();

    EXPECT_EQ(kItems, consumed.load());
}

// ---------------------------------------------------------------------------
// BoundedBlockingQueue
// ---------------------------------------------------------------------------
TEST(ConcurrencyTest, BoundedBlockingQueueRoundTrip)
{
    BoundedBlockingQueue<int> queue(3);
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_TRUE(queue.push(3));
    EXPECT_EQ(3u, queue.size());

    int v = 0;
    EXPECT_TRUE(queue.pop(v));
    EXPECT_EQ(1, v);
    EXPECT_TRUE(queue.pop(v));
    EXPECT_EQ(2, v);
}

TEST(ConcurrencyTest, BoundedBlockingQueueBlocksWhenFull)
{
    BoundedBlockingQueue<int> queue(2);
    queue.push(1);
    queue.push(2);
    EXPECT_EQ(2u, queue.size());

    // 生产者在队列满时阻塞，消费者取走后才能继续
    std::thread producer([&] { EXPECT_TRUE(queue.push(3)); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int v = 0;
    EXPECT_TRUE(queue.pop(v));
    producer.join();
    // pop 取走 1 个后 producer 才能补回 1 个：2 - 1 + 1 = 2
    EXPECT_EQ(2u, queue.size());
}

// ---------------------------------------------------------------------------
// ThreadPool
// ---------------------------------------------------------------------------
TEST(ConcurrencyTest, ThreadPoolConcurrentTasks)
{
    ThreadPool pool("test-pool");
    pool.start(4);

    std::atomic<int> done(0);
    CountDownLatch latch(100);
    for (int i = 0; i < 100; ++i)
    {
        pool.run([&] {
            done.fetch_add(1);
            latch.countDown();
        });
    }
    latch.wait();
    EXPECT_EQ(100, done.load());

    pool.stop();
}

// ---------------------------------------------------------------------------
// CountDownLatch
// ---------------------------------------------------------------------------
TEST(ConcurrencyTest, CountDownLatchWait)
{
    CountDownLatch latch(3);
    std::atomic<int> finished(0);

    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i)
    {
        threads.emplace_back([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            finished.fetch_add(1);
            latch.countDown();
        });
    }

    latch.wait();
    EXPECT_EQ(3, finished.load());
    EXPECT_EQ(0, latch.getCount());
    for (auto& t : threads)
        t.join();
}

// ---------------------------------------------------------------------------
// ConcurrentQueue
// ---------------------------------------------------------------------------
TEST(ConcurrencyTest, ConcurrentQueueMultiThreadCount)
{
    ConcurrentQueue<int> queue;
    const int kPerThread = 2000;
    const int kThreads = 4;

    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t)
    {
        producers.emplace_back([&] {
            for (int i = 0; i < kPerThread; ++i)
                queue.push(i);
        });
    }
    for (auto& t : producers)
        t.join();

    // 全部弹出后为空（无 size() 接口，用累计弹出数校验）
    int v = 0;
    size_t popped = 0;
    while (queue.pop(v))
        ++popped;
    EXPECT_EQ(static_cast<size_t>(kPerThread * kThreads), popped);
    EXPECT_TRUE(queue.empty());
}

#include "afl/concurrency/RWMutex.h"
#include "afl/concurrency/ConcurrentHashMap.h"

TEST(ConcurrencyTest, RWMutexReadWrite)
{
    RWMutex mu;
    EXPECT_TRUE(mu.writeLock());
    EXPECT_TRUE(mu.writeUnLock());
    EXPECT_TRUE(mu.readLock());
    EXPECT_TRUE(mu.readUnLock());
    EXPECT_TRUE(mu.tryWriteLock());
    EXPECT_TRUE(mu.writeUnLock());
}

TEST(ConcurrencyTest, ConcurrentHashMapBasic)
{
    ConcurrentHashMap<std::string, int> map;
    map.put("a", 1);
    map.put("b", 2);
    EXPECT_TRUE(map.contain("a"));
    EXPECT_EQ(1, map.get("a"));
    EXPECT_EQ(2, map.get("b"));
    EXPECT_EQ(0, map.get("missing", 0));
    EXPECT_EQ(2u, map.size());
    EXPECT_EQ(2, map.remove("b"));
    EXPECT_FALSE(map.contain("b"));

    EXPECT_EQ(1, map.putIfAbsent("a", 99));
    EXPECT_EQ(5, map.putIfAbsent("c", 5));
    EXPECT_TRUE(map.remove("c", 5));
    EXPECT_FALSE(map.contain("c"));

    map.put("x", 10);
    map.put("y", 20);
    std::map<std::string, int> snapshot = map.toMap();
    EXPECT_EQ(10, snapshot["x"]);
    EXPECT_EQ(20, snapshot["y"]);
}

TEST(ConcurrencyTest, ConcurrentHashMapMultiThread)
{
    ConcurrentHashMap<int, int> map(31);
    const int kPerThread = 500;
    const int kThreads = 4;
    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t)
    {
        workers.emplace_back([&, t] {
            for (int i = 0; i < kPerThread; ++i)
            {
                const int key = t * kPerThread + i;
                map.put(key, key);
            }
        });
    }
    for (auto& th : workers)
    {
        th.join();
    }
    EXPECT_EQ(static_cast<size_t>(kPerThread * kThreads), map.size());
    EXPECT_EQ(0, map.get(0));
    EXPECT_EQ(kPerThread * kThreads - 1, map.get(kPerThread * kThreads - 1));
}
