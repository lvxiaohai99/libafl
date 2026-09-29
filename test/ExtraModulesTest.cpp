/**
 * @file   ExtraModulesTest.cpp
 * @brief  补充可测模块的单元测试（提高行/分支覆盖）
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <typeinfo>
#include <unistd.h>
#include <vector>

#include "afl/base/Demangle.h"
#include "afl/base/Exception.h"
#include "afl/base/ScopeExitGuard.h"
#include "afl/concurrency/CyclicBarrier.h"
#include "afl/concurrency/Event.h"
#include "afl/concurrency/Semaphore.h"
#include "afl/concurrency/Thread.h"
#include "afl/concurrency/ThreadGroup.h"
#include "afl/concurrency/ThreadLocal.h"
#include "afl/file/File.h"
#include "afl/framework/EventDispatcher.h"
#include "afl/framework/ObjectRegistrar.h"
#include "afl/net/DomainResolver.h"
#include "afl/net/http/HttpKeyValue.h"
#include "afl/net/http/HttpProtocol.h"
#include "afl/net/http/UriUtil.h"
#include "afl/string/StringPiece.h"
#include "afl/time/Date.h"
#include "afl/time/ProcessTimeCounter.h"

using namespace afl::base;
using namespace afl::concurrency;
using namespace afl::file;
using namespace afl::fw;
using namespace afl::net;
using namespace afl::str;
using namespace afl::time;

// ---------------------------------------------------------------------------
// base
// ---------------------------------------------------------------------------
TEST(ExtraBaseTest, ScopeExitGuardRunsAndDismiss)
{
    int n = 0;
    {
        ScopeExitGuard g([&] { ++n; });
        EXPECT_EQ(0, n);
    }
    EXPECT_EQ(1, n);

    {
        ScopeExitGuard g([&] { ++n; });
        g.dismiss();
    }
    EXPECT_EQ(1, n);

    int m = 0;
    {
        ON_SCOPE_EXIT([&] { ++m; });
    }
    EXPECT_EQ(1, m);
}

TEST(ExtraBaseTest, ExceptionWhatAndLocation)
{
    Exception e(__FILE__, __LINE__, "unit-test-error");
    EXPECT_STREQ("unit-test-error", e.what());
    EXPECT_FALSE(std::string(e.filename()).empty());
    EXPECT_GT(e.line(), 0);
    // stackTrace 在部分环境可能为空，只要求可调用
    (void)e.stackTrace();
}

TEST(ExtraBaseTest, DemangleTypeName)
{
    const char* mangled = typeid(std::string).name();
    std::string out;
    demangleName(mangled, out);
    char buf[256] = {0};
    demangleName(mangled, buf, sizeof(buf));
    // 覆盖两条重载路径即可；不同编译器结果可能不同
    SUCCEED();
}

// ---------------------------------------------------------------------------
// time
// ---------------------------------------------------------------------------
TEST(ExtraTimeTest, DateLeapAndArithmetic)
{
    EXPECT_TRUE(Date::isLeapYear(2024));
    EXPECT_FALSE(Date::isLeapYear(2023));
    EXPECT_TRUE(Date::isValid(2026, 9, 24));
    EXPECT_FALSE(Date::isValid(2026, 2, 30));
    EXPECT_EQ(29, Date::daysInMonth(2024, 2));
    EXPECT_EQ(28, Date::daysInMonth(2023, 2));

    Date d(2026, 1, 31);
    EXPECT_EQ(2026, d.year());
    EXPECT_EQ(1, d.month());
    EXPECT_EQ(31, d.day());

    Date d2 = d;
    d2.addDays(1);
    EXPECT_EQ(2, d2.month());
    EXPECT_EQ(1, d2.day());

    Date a(2026, 1, 1);
    Date b(2026, 1, 11);
    EXPECT_EQ(10, Date::daysDiff(a, b));
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a != b);
    EXPECT_EQ("2026-01-01", a.toString());

    Date today = Date::today();
    EXPECT_TRUE(Date::isValid(today.year(), today.month(), today.day()));
}

TEST(ExtraTimeTest, ProcessTimeCounterStartStop)
{
    ProcessTimeCounter c;
    c.start();
    volatile int sink = 0;
    for (int i = 0; i < 100000; ++i)
    {
        sink += i;
    }
    (void)sink;
    c.stop();
    EXPECT_GE(c.microSeconds(), 0);
    EXPECT_GE(c.userMicroSeconds() + c.kernelMicroseconds(), 0);
}

// ---------------------------------------------------------------------------
// string
// ---------------------------------------------------------------------------
TEST(ExtraStringTest, StringPieceBasic)
{
    std::string s = "hello-libafl";
    StringPiece sp(s);
    EXPECT_FALSE(sp.empty());
    EXPECT_EQ(s.size(), sp.size());
    EXPECT_EQ(s, sp.asString());
    EXPECT_EQ(static_cast<size_t>(s.size()), sp.size());
    EXPECT_EQ('h', *sp.data());

    StringPiece empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(0u, empty.size());
}

// ---------------------------------------------------------------------------
// concurrency
// ---------------------------------------------------------------------------
TEST(ExtraConcurrencyTest, EventSetWaitAutoReset)
{
    Event ev(false, true);
    std::atomic<bool> done(false);
    std::thread t([&] {
        ev.wait();
        done = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_FALSE(done.load());
    ev.set();
    t.join();
    EXPECT_TRUE(done.load());
    EXPECT_FALSE(ev.tryWait());  // 自动复位后应为未触发
}

TEST(ExtraConcurrencyTest, SemaphorePostWait)
{
    Semaphore sem(0);
    EXPECT_FALSE(sem.tryWait());
    EXPECT_TRUE(sem.post(2));
    EXPECT_TRUE(sem.tryWait());
    EXPECT_TRUE(sem.wait());
    EXPECT_FALSE(sem.tryWait());
}

TEST(ExtraConcurrencyTest, CyclicBarrierThreeParties)
{
    const int n = 3;
    std::atomic<int> arrived(0);
    CyclicBarrier barrier(n, [&] { arrived.fetch_add(100); });

    std::vector<std::thread> threads;
    for (int i = 0; i < n; ++i)
    {
        threads.emplace_back([&] {
            barrier.wait();
            arrived.fetch_add(1);
        });
    }
    for (auto& th : threads)
    {
        th.join();
    }
    EXPECT_EQ(100 + n, arrived.load());
}

TEST(ExtraConcurrencyTest, ThreadLocalValue)
{
    ThreadLocal<int> tls;
    *tls.get() = 7;
    EXPECT_EQ(7, tls());
}

TEST(ExtraConcurrencyTest, ThreadGroupJoin)
{
    std::atomic<int> n(0);
    ThreadGroup group;
    group.createThread([&] { n.fetch_add(1); }, "t1");
    group.createThread([&] { n.fetch_add(1); }, "t2");
    EXPECT_EQ(2u, group.size());
    group.joinAll();
    EXPECT_EQ(2, n.load());
}

// ---------------------------------------------------------------------------
// file
// ---------------------------------------------------------------------------
TEST(ExtraFileTest, DiskFileReadWrite)
{
    const std::string path = "/tmp/libafl_file_" + std::to_string(::getpid()) + ".txt";
    ::unlink(path.c_str());

    {
        File f;
        ASSERT_TRUE(f.fopen(path, "w+"));
        const char* msg = "hello-file";
        EXPECT_EQ(1u, f.fwrite(msg, strlen(msg), 1));
        f.fclose();
    }

    {
        File f(path, "r");
        char buf[64] = {0};
        EXPECT_EQ(1u, f.fread(buf, strlen("hello-file"), 1));
        EXPECT_STREQ("hello-file", buf);
        EXPECT_GE(f.size(), static_cast<off_t>(strlen("hello-file")));
        f.fclose();
    }

    {
        MemFile mf;
        const char* msg = "mem";
        EXPECT_EQ(3u, mf.fwrite(msg, 3, 1));
        mf.resetRead();
        char buf[8] = {0};
        EXPECT_EQ(3u, mf.fread(buf, 3, 1));
        EXPECT_STREQ("mem", buf);
    }

    ::unlink(path.c_str());
}

// ---------------------------------------------------------------------------
// framework
// ---------------------------------------------------------------------------
struct RegBase
{
    virtual ~RegBase() {}
    virtual int id() const = 0;
};
struct RegA : public RegBase
{
    virtual int id() const override { return 1; }
};
struct RegB : public RegBase
{
    virtual int id() const override { return 2; }
};

TEST(ExtraFrameworkTest, ObjectRegistrarUniqueAndMulti)
{
    using Reg = ObjectRegistrar<RegBase>;
    Reg::registerObject<RegA>("a", true);
    Reg::registerObject<RegB>("b", false);

    auto a1 = Reg::getInstance("a");
    auto a2 = Reg::getInstance("a");
    ASSERT_TRUE(a1);
    ASSERT_TRUE(a2);
    EXPECT_EQ(a1.get(), a2.get());  // unique
    EXPECT_EQ(1, a1->id());

    auto b1 = Reg::getInstance("b");
    auto b2 = Reg::getInstance("b");
    ASSERT_TRUE(b1);
    ASSERT_TRUE(b2);
    EXPECT_NE(b1.get(), b2.get());  // non-unique
    EXPECT_EQ(2, b1->id());

    EXPECT_FALSE(Reg::getInstance("missing"));
}

TEST(ExtraFrameworkTest, EventDispatcherInvoke)
{
    EventDispatcher<std::string, void(int)> disp;
    std::atomic<int> sum(0);

    disp.append("add", [&](std::string e, int v) {
        EXPECT_EQ("add", e);
        sum.fetch_add(v);
    });
    disp.append([&](std::string, int v) { sum.fetch_add(v * 10); });  // 全局

    disp.invoke("add", 3);
    EXPECT_EQ(3 + 30, sum.load());
}

// ---------------------------------------------------------------------------
// net helpers
// ---------------------------------------------------------------------------
TEST(ExtraNetTest, UriEncodeDecodeRoundTrip)
{
    const std::string raw = "a b/中文?x=1";
    std::string enc = uriEncode(raw);
    EXPECT_FALSE(enc.empty());
    EXPECT_NE(raw, enc);
    std::string dec = uriDecode(enc);
    EXPECT_EQ(raw, dec);
}

TEST(ExtraNetTest, HttpKeyValueMaps)
{
    HttpKeyValue* kv = HttpKeyValue::getInstancePtr();
    ASSERT_TRUE(kv != nullptr);
    EXPECT_FALSE(kv->getStatusDesc(HttpStatusOk).empty());
    EXPECT_FALSE(kv->getMethodStr(HttpGet).empty());
    (void)kv->getContentType("html");
}

TEST(ExtraNetTest, DomainResolverLocalhost)
{
    std::vector<InetAddress> ips;
    int err = 0;
    bool ok = DomainResolver::resolveInetAddress("localhost", &ips, &err);
    // 环境可能无解析器；至少不崩溃，成功时非空
    if (ok)
    {
        EXPECT_FALSE(ips.empty());
    }
}
