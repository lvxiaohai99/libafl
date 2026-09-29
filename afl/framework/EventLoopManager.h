/**
 * @file   EventLoopManager.h
 * @brief  多线程 EventLoop 创建与调度入口
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/NonCopy.h"
#include "afl/net/EventLoop.h"
#include <memory>

namespace afl
{
namespace fw
{
class EventLoopManagerImpl;

/** @brief 多 EventLoop 线程的创建、命名与当前线程查询 */
class EventLoopManager final : public afl::base::NonCopy
{
    struct ThreadAttribute
    {
        bool unique;
        int policy;
        int priority;
        int stackSize;
    };

public:
    EventLoopManager();

    void run();
    void quit();

    /**
     * @brief 为命名 EventLoop 配置线程属性（栈、调度策略等）
     * @param name 索引名（绑定到专用线程）
     * @param unique true：同名复用同一实例（推荐）；false：每次 getEventLoop 新建线程/实例
     * @param stackSize 线程栈大小，单位 KB（如 64 表示 65536 字节）
     * @param policy SCHED_FIFO / SCHED_RR / SCHED_OTHER
     * @param priority 实时调度优先级，常见合法范围 [1, 99]
     * @return 0 成功；-1 名称为空；-2 调度策略非法
     */
    int createCustomAttributes(const std::string& name, bool unique, int stackSize = 256,
                               int policy = SCHED_OTHER, int priority = 0);

    /**
     * @brief 按名称获取 EventLoop（未配置则为主循环）
     * @param name 索引名，空串表示主循环
     * @return EventLoop 共享指针
     */
    std::shared_ptr<afl::net::EventLoop> getEventLoop(const std::string& name = "");

    /**
     * @brief 获取当前线程绑定的 EventLoop
     * @return 当前线程 EventLoop，未绑定则为空
     */
    std::shared_ptr<afl::net::EventLoop> getCurrentEventLoop();

private:
    std::shared_ptr<EventLoopManagerImpl> m_evmImpl;
};

} // namespace fw
} // namespace afl
