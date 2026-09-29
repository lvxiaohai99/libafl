/**
 * @file   EventLoopManager.cpp
 * @brief  EventLoop 管理器门面实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/framework/EventLoopManager.h"
#include "afl/framework/details/EventLoopManagerImpl.h"

namespace afl
{
namespace fw
{
EventLoopManager::EventLoopManager()
{
    m_evmImpl = std::make_shared<EventLoopManagerImpl>();
}

void EventLoopManager::run()
{
    m_evmImpl->run();
}

void EventLoopManager::quit()
{
    m_evmImpl->quit();
}

int EventLoopManager::createCustomAttributes(const std::string& name, bool unique, int stackSize,
                                             int policy, int priority)
{
    return m_evmImpl->createCustomAttributes(name, unique, stackSize, policy, priority);
}

std::shared_ptr<afl::net::EventLoop> EventLoopManager::getEventLoop(const std::string& name)
{
    return m_evmImpl->getEventLoop(name);
}

std::shared_ptr<afl::net::EventLoop> EventLoopManager::getCurrentEventLoop()
{
    return m_evmImpl->getCurrentEventLoop();
}

} // namespace fw
} // namespace afl
