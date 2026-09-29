/**
 * @file   FrameworkTest.cpp
 * @brief  framework 模块单元测试：Module / ModuleManager 注册与生命周期
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "afl/framework/Module.h"
#include "afl/framework/ModuleManager.h"

using namespace afl::fw;

namespace
{
/// 记录生命周期调用序列的测试模块
class TestModule : public Module
{
public:
    TestModule() = default;
    explicit TestModule(std::vector<std::string>* log, std::string tag)
        : m_log(log), m_tag(std::move(tag))
    {
    }

    void setRecorder(std::vector<std::string>* log, std::string tag)
    {
        m_log = log;
        m_tag = std::move(tag);
    }

protected:
    virtual bool doInit() override
    {
        if (m_log)
            m_log->push_back(m_tag + ":init");
        return true;
    }
    virtual void doDeinit() override
    {
        if (m_log)
            m_log->push_back(m_tag + ":deinit");
    }
    virtual bool doStart() override
    {
        if (m_log)
            m_log->push_back(m_tag + ":start");
        return true;
    }
    virtual void doStop() override
    {
        if (m_log)
            m_log->push_back(m_tag + ":stop");
    }

private:
    std::vector<std::string>* m_log = nullptr;
    std::string m_tag;
};

} // namespace

// ---------------------------------------------------------------------------
// Module 生命周期
// ---------------------------------------------------------------------------
TEST(FrameworkTest, ModuleLifecycle)
{
    std::vector<std::string> log;
    TestModule m;
    m.setRecorder(&log, "m");

    EXPECT_TRUE(m.init());
    EXPECT_TRUE(m.start());
    m.stop();
    m.deinit();

    ASSERT_EQ(4u, log.size());
    EXPECT_EQ("m:init", log[0]);
    EXPECT_EQ("m:start", log[1]);
    EXPECT_EQ("m:stop", log[2]);
    EXPECT_EQ("m:deinit", log[3]);
}

// ---------------------------------------------------------------------------
// ModuleManager 注册 / 查找 / 遍历次序
// ---------------------------------------------------------------------------
TEST(FrameworkTest, ModuleManagerRegisterAndOrder)
{
    ModuleManager mgr;
    std::vector<std::string> log;

    // registerModule<D>(key, serial) —— serial 越小越先被 forEach 遍历
    mgr.registerModule<TestModule>("mod-b", 20);
    mgr.registerModule<TestModule>("mod-a", 10);

    EXPECT_EQ(2u, mgr.count());
    EXPECT_TRUE(mgr.isExist("mod-a"));
    EXPECT_TRUE(mgr.isExist("mod-b"));
    EXPECT_FALSE(mgr.isExist("mod-none"));

    // getModuleInstance 类型正确
    auto inst = mgr.getModuleInstance<TestModule>("mod-a");
    ASSERT_TRUE(inst != nullptr);
    inst->setRecorder(&log, "a");

    // forEach 按 serial 升序：a(10) 在 b(20) 之前
    std::vector<std::string> visited;
    mgr.forEach([&](std::shared_ptr<Module> m) {
        visited.push_back(m->getName());
        auto* tm = dynamic_cast<TestModule*>(m.get());
        if (tm && m->getName() == "mod-a")
            tm->setRecorder(&log, "a");
    });
    ASSERT_EQ(2u, visited.size());
    EXPECT_EQ("mod-a", visited[0]);
    EXPECT_EQ("mod-b", visited[1]);

    // forEachReverse 反序
    visited.clear();
    mgr.forEachReverse([&](std::shared_ptr<Module> m) { visited.push_back(m->getName()); });
    EXPECT_EQ("mod-b", visited[0]);
    EXPECT_EQ("mod-a", visited[1]);

    // 注销
    mgr.unregisterModule("mod-a");
    EXPECT_EQ(1u, mgr.count());
    EXPECT_FALSE(mgr.isExist("mod-a"));
    mgr.clear();
    EXPECT_EQ(0u, mgr.count());
}

TEST(FrameworkTest, ModuleManagerFullLifecycleOrder)
{
    ModuleManager mgr;
    std::vector<std::string> log;

    // 用生成器注入带记录器的模块
    mgr.registerModule("l1", 10, [&] { return std::make_shared<TestModule>(&log, "l1"); });
    mgr.registerModule("l2", 20, [&] { return std::make_shared<TestModule>(&log, "l2"); });

    // init 全部（serial 升序）
    mgr.forEach([](std::shared_ptr<Module> m) { m->init(); });
    mgr.forEach([](std::shared_ptr<Module> m) { m->start(); });
    // 停止与初始化相反次序
    mgr.forEachReverse([](std::shared_ptr<Module> m) { m->stop(); });
    mgr.forEachReverse([](std::shared_ptr<Module> m) { m->deinit(); });

    ASSERT_EQ(8u, log.size());
    EXPECT_EQ("l1:init", log[0]);
    EXPECT_EQ("l2:init", log[1]);
    EXPECT_EQ("l1:start", log[2]);
    EXPECT_EQ("l2:start", log[3]);
    EXPECT_EQ("l2:stop", log[4]);
    EXPECT_EQ("l1:stop", log[5]);
    EXPECT_EQ("l2:deinit", log[6]);
    EXPECT_EQ("l1:deinit", log[7]);
}
