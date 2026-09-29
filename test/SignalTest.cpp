/**
 * @file   SignalTest.cpp
 * @brief  base::Signal 信号/槽单元测试
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "afl/base/Signal.h"

using afl::base::Connection;
using afl::base::ScopedConnection;
using afl::base::Signal;

TEST(SignalTest, ConnectEmitDisconnect)
{
    Signal<void(int)> sig;
    int sum = 0;
    Connection c = sig.connect([&](int v) { sum += v; });
    EXPECT_TRUE(c.connected());
    EXPECT_EQ(1u, sig.slotCount());

    sig(3);
    sig(7);
    EXPECT_EQ(10, sum);

    c.disconnect();
    EXPECT_FALSE(c.connected());
    EXPECT_EQ(0u, sig.slotCount());
    sig(100);
    EXPECT_EQ(10, sum);
}

TEST(SignalTest, MultiSlotsAndScopedConnection)
{
    Signal<void(const std::string&)> sig;
    std::vector<std::string> got;

    ScopedConnection c1 = sig.connect([&](const std::string& s) { got.push_back("A:" + s); });
    Connection c2 = sig.connect([&](const std::string& s) { got.push_back("B:" + s); });

    sig("x");
    ASSERT_EQ(2u, got.size());
    EXPECT_EQ("A:x", got[0]);
    EXPECT_EQ("B:x", got[1]);

    c1.reset();
    got.clear();
    sig("y");
    ASSERT_EQ(1u, got.size());
    EXPECT_EQ("B:y", got[0]);

    c2.disconnect();
    EXPECT_TRUE(sig.empty());
}

TEST(SignalTest, DisconnectAllSlots)
{
    Signal<void()> sig;
    int n = 0;
    sig.connect([&] { ++n; });
    sig.connect([&] { ++n; });
    EXPECT_EQ(2u, sig.slotCount());
    sig.disconnectAllSlots();
    EXPECT_TRUE(sig.empty());
    sig();
    EXPECT_EQ(0, n);
}

TEST(SignalTest, EmitAllowsReconnect)
{
    Signal<void()> sig;
    int n = 0;
    Connection inner;
    sig.connect([&] {
        ++n;
        if (!inner.connected())
        {
            inner = sig.connect([&] { n += 10; });
        }
    });
    sig();
    EXPECT_EQ(1, n);
    sig();
    EXPECT_EQ(12, n);  // 原槽 +10
}

TEST(SignalTest, MultiThreadEmit)
{
    Signal<void(int)> sig;
    std::atomic<int> sum(0);
    sig.connect([&](int v) { sum.fetch_add(v); });

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([&] {
            for (int i = 0; i < 100; ++i)
            {
                sig(1);
            }
        });
    }
    for (auto& th : threads)
    {
        th.join();
    }
    EXPECT_EQ(400, sum.load());
}

/** 模拟 MetaEvent::infuse → m_Signal(...) 的用法 */
TEST(SignalTest, MetaEventStyleUsage)
{
    Signal<void(int, int, const char* const, int)> signal;
    int lastLen = -1;
    auto conn = signal.connect([&](int, int, const char* const, int len) { lastLen = len; });

    const char* payload = "dummy";
    signal(1, 2, payload, 5);
    EXPECT_EQ(5, lastLen);
    (void)conn;
}
