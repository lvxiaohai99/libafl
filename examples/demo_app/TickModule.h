/**
 * @file   TickModule.h
 * @brief  心跳模块：EventLoop 定时器 + Signal 发射
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Signal.h"
#include "afl/framework/Module.h"
#include "afl/log/Log.h"
#include "afl/net/EventLoop.h"

#include <functional>
#include <memory>

namespace demo
{

/**
 * @brief 按间隔发射 tick 信号；达到上限时回调 onFinished
 */
class TickModule : public afl::fw::Module
{
public:
    using FinishedCallback = std::function<void()>;

    TickModule(std::shared_ptr<afl::net::EventLoop> loop, double intervalSec, int maxTicks,
               FinishedCallback onFinished)
        : m_loop(std::move(loop))
        , m_intervalSec(intervalSec)
        , m_maxTicks(maxTicks)
        , m_onFinished(std::move(onFinished))
    {
    }

    /** @brief 其它模块可 connect 此信号：参数为当前 tick 序号（从 1 起） */
    afl::base::Signal<void(int)>& tickSignal() { return m_tickSignal; }

protected:
    bool doInit() override
    {
        LOG_INFO("[TickModule] init interval=%.3fs maxTicks=%d", m_intervalSec, m_maxTicks);
        return true;
    }

    void doDeinit() override { LOG_INFO("[TickModule] deinit"); }

    bool doStart() override
    {
        if (!m_loop)
        {
            LOG_ERROR("[TickModule] EventLoop is null");
            return false;
        }
        m_tickCount = 0;
        m_timerId = m_loop->addTimer([this] { onTimer(); }, m_intervalSec, true);
        LOG_INFO("[TickModule] start timer");
        return true;
    }

    void doStop() override
    {
        if (m_loop && m_timerId)
        {
            m_loop->cancelTimer(m_timerId);
            m_timerId = 0;
        }
        LOG_INFO("[TickModule] stop");
    }

private:
    void onTimer()
    {
        ++m_tickCount;
        LOG_INFO("[TickModule] emit tick=%d", m_tickCount);
        m_tickSignal(m_tickCount);

        if (m_maxTicks > 0 && m_tickCount >= m_maxTicks)
        {
            LOG_INFO("[TickModule] reached maxTicks=%d, finish", m_maxTicks);
            if (m_loop && m_timerId)
            {
                m_loop->cancelTimer(m_timerId);
                m_timerId = 0;
            }
            if (m_onFinished)
            {
                m_onFinished();
            }
        }
    }

    std::shared_ptr<afl::net::EventLoop> m_loop;
    double m_intervalSec = 1.0;
    int m_maxTicks = 0;
    FinishedCallback m_onFinished;
    afl::base::Signal<void(int)> m_tickSignal;
    int m_tickCount = 0;
    afl::net::TimerId m_timerId = 0;
};

} // namespace demo
