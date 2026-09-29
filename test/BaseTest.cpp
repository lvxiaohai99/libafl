/**
 * @file   BaseTest.cpp
 * @brief  base 模块单元测试：Singleton / ObjectPool / Closure / Any
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <set>

#include "afl/base/Singleton.h"
#include "afl/base/ObjectPool.h"
#include "afl/base/Closure.h"
#include "afl/base/Any.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
class SingletonTarget
{
public:
    SingletonTarget() { ++constructCount; }
    static std::atomic<int> constructCount;
};
std::atomic<int> SingletonTarget::constructCount(0);

TEST(BaseTest, SingletonUniqueInMultiThread)
{
    SingletonTarget::constructCount = 0;
    std::vector<std::thread> threads;
    std::set<SingletonTarget*> pointers;
    afl::concurrency::Mutex mutex;
    for (int i = 0; i < 8; ++i)
    {
        threads.emplace_back([&] {
            SingletonTarget* p = afl::base::Singleton<SingletonTarget>::getInstancePtr();
            {
                afl::concurrency::LockGuard<afl::concurrency::Mutex> lock(mutex);
                pointers.insert(p);
            }
        });
    }
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(1u, pointers.size());
    EXPECT_EQ(1, SingletonTarget::constructCount.load());
    // getInstance 与 getInstancePtr 返回同一实例
    EXPECT_EQ(afl::base::Singleton<SingletonTarget>::getInstancePtr(),
              &afl::base::Singleton<SingletonTarget>::getInstance());

    afl::base::Singleton<SingletonTarget>::deleteInstance();
    EXPECT_EQ(1, SingletonTarget::constructCount.load()); // delete 后不再增长
}

// ---------------------------------------------------------------------------
// ObjectPool
// ---------------------------------------------------------------------------
TEST(BaseTest, ObjectPoolAllocFree)
{
    afl::base::ObjectPool<int> pool(4, 0);
    EXPECT_EQ(4, pool.avail());
    EXPECT_EQ(4, pool.total());

    int* a = pool.alloc();
    ASSERT_TRUE(a != NULL);
    EXPECT_EQ(3, pool.avail());

    *a = 42;
    pool.free(a);
    EXPECT_EQ(4, pool.avail());

    // 取尽后自动扩容（每次扩容固定 size_per_alloc = 128）
    std::vector<int*> objs;
    for (int i = 0; i < 10; ++i)
    {
        int* p = pool.alloc();
        ASSERT_TRUE(p != NULL);
        objs.push_back(p);
    }
    EXPECT_EQ(4 + 128, pool.total());
    for (auto* p : objs)
        pool.free(p);
    EXPECT_EQ(4 + 128, pool.avail());
}

// ---------------------------------------------------------------------------
// Closure
// ---------------------------------------------------------------------------
static int g_closureValue = 0;
static void closureFunction()
{
    g_closureValue += 5;
}

TEST(BaseTest, ClosureRun)
{
    g_closureValue = 0;
    afl::Closure* cb = afl::GenCallback(&closureFunction);
    cb->run(); // self-delete 版本：run 后自动析构
    EXPECT_EQ(5, g_closureValue);
}

TEST(BaseTest, ClosureMemberMethod)
{
    struct Runner
    {
        int value = 0;
        void add() { value += 7; }
    } r;

    afl::Closure* cb = afl::GenPermanentCallback(&r, &Runner::add);
    cb->run();
    cb->run();
    EXPECT_EQ(14, r.value);
    delete cb;
}

// ---------------------------------------------------------------------------
// Any
// ---------------------------------------------------------------------------
TEST(BaseTest, AnyStoreAndCast)
{
    afl::base::any a;
    EXPECT_TRUE(a.empty());

    a = 42;
    EXPECT_FALSE(a.empty());
    EXPECT_EQ(42, *afl::base::any_cast<int>(&a));

    a = std::string("hello");
    EXPECT_EQ("hello", *afl::base::any_cast<std::string>(&a));

    a = 3.14;
    EXPECT_DOUBLE_EQ(3.14, *afl::base::any_cast<double>(&a));

    // 错误类型转换返回 NULL
    EXPECT_TRUE(afl::base::any_cast<int>(&a) == NULL);
}
