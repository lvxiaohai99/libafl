/**
 * @file   StatsModule.h
 * @brief  统计模块：订阅 TickModule 信号并打日志
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "TickModule.h"

#include "afl/base/Signal.h"
#include "afl/framework/Module.h"
#include "afl/log/Log.h"

#include <memory>

namespace demo
{

/**
 * @brief 演示模块间通过 Signal 通信
 */
class StatsModule : public afl::fw::Module
{
public:
    explicit StatsModule(std::shared_ptr<TickModule> tick) : m_tick(std::move(tick)) {}

    int receivedCount() const { return m_received; }

protected:
    bool doInit() override
    {
        if (!m_tick)
        {
            LOG_ERROR("[StatsModule] TickModule is null");
            return false;
        }
        m_conn = m_tick->tickSignal().connect([this](int n) {
            ++m_received;
            LOG_INFO("[StatsModule] received tick=%d (total=%d)", n, m_received);
        });
        LOG_INFO("[StatsModule] init, connected to TickModule");
        return true;
    }

    void doDeinit() override
    {
        m_conn.disconnect();
        LOG_INFO("[StatsModule] deinit, received=%d", m_received);
    }

    bool doStart() override
    {
        LOG_INFO("[StatsModule] start");
        return true;
    }

    void doStop() override { LOG_INFO("[StatsModule] stop"); }

private:
    std::shared_ptr<TickModule> m_tick;
    afl::base::Connection m_conn;
    int m_received = 0;
};

} // namespace demo
