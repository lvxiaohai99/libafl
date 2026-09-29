/**
 * @file   EventLoopManagerImpl.h
 * @brief  EventLoop 线程创建与映射表
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/net/EventLoop.h"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace afl
{
namespace fw
{
class EventLoopManagerImpl
{
    struct ThreadAttribute
    {
        bool unique;
        int policy;
        int priority;
        int stackSize;
    };

public:
    EventLoopManagerImpl();
    virtual ~EventLoopManagerImpl();

    void run();
    void quit();

    /**
     * @brief 为命名 EventLoop 配置线程属性
     * @param name 索引名
     * @param unique 是否每次获取时创建新实例
     * @param stackSize 栈大小（KB）
     * @param policy 调度策略
     * @param priority 实时优先级
     * @return 0 成功；-1 名称为空；-2 策略非法
     */
    int createCustomAttributes(const std::string& name, bool unique, int stackSize = 256,
                               int policy = SCHED_OTHER, int priority = 0);

    /**
     * @brief 按名称获取 EventLoop
     * @param name 索引名
     * @return EventLoop 实例
     */
    std::shared_ptr<afl::net::EventLoop> getEventLoop(const std::string& name = "");

    /**
     * @brief 当前 pthread 绑定的 EventLoop
     * @return EventLoop 或 nullptr
     */
    std::shared_ptr<afl::net::EventLoop> getCurrentEventLoop();

private:
    std::shared_ptr<afl::net::EventLoop> createEventLoop(const ThreadAttribute& attr);

    static void* createEventLoopDetail(void* arg);
    static void installTraceBackSignals();
    static void traceBack(int signo);

    inline size_t getUpperLimit(int type, size_t dft);

private:
    static std::shared_ptr<afl::net::EventLoop> s_MainLoop;

    std::mutex s_Thread2EventLoopMapLock;
    std::mutex s_Name2EventLoopMapLock;
    std::mutex s_ModuleThreadAttrsLock;

    /** @brief pthread_t → EventLoop，供 getCurrentEventLoop 查询 */
    std::unordered_map<pthread_t, std::weak_ptr<afl::net::EventLoop>> s_Thread2EventLoopMap;
    /** @brief 名称 → EventLoop，供 getEventLoop(name) 查询 */
    std::unordered_map<std::string, std::weak_ptr<afl::net::EventLoop>> s_Name2EventLoopMap;
    /** @brief 名称 → 线程属性，供 createEventLoop 使用 */
    std::unordered_map<std::string, ThreadAttribute> s_ModuleThreadAttrs;
};

} // namespace fw
} // namespace afl
