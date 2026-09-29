/**
 * @file   Module.cpp
 * @brief  模块生命周期实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/framework/Module.h"
#include "afl/log/Log.h"

namespace afl
{
namespace fw
{
namespace
{
const char* stateName(Module::ModuleState s)
{
    switch (s)
    {
    case Module::MsInPlace:
        return "InPlace";
    case Module::MsReady:
        return "Ready";
    case Module::MsWorking:
        return "Working";
    case Module::MsPause:
        return "Pause";
    default:
        return "?";
    }
}
} // namespace

bool Module::init()
{
    if (MsInPlace != m_state)
    {
        LOG_WARN("Module[%s] init skipped, state=%s", m_name.c_str(), stateName(m_state));
        return false;
    }
    LOG_DEBUG("Module[%s] init ...", m_name.c_str());
    if (!doInit())
    {
        LOG_ERROR("Module[%s] doInit failed", m_name.c_str());
        return false;
    }
    m_state = MsReady;
    LOG_INFO("Module[%s] init ok -> Ready", m_name.c_str());
    return true;
}

void Module::deinit()
{
    if (MsPause == m_state)
    {
        resume();
    }

    if (MsWorking == m_state)
    {
        stop();
    }

    if (MsReady == m_state)
    {
        LOG_DEBUG("Module[%s] deinit ...", m_name.c_str());
        doDeinit();
        m_state = MsInPlace;
        LOG_INFO("Module[%s] deinit ok -> InPlace", m_name.c_str());
    }
}

bool Module::start()
{
    if (MsReady != m_state)
    {
        LOG_WARN("Module[%s] start skipped, state=%s", m_name.c_str(), stateName(m_state));
        return false;
    }
    LOG_DEBUG("Module[%s] start ...", m_name.c_str());
    if (!doStart())
    {
        LOG_ERROR("Module[%s] doStart failed", m_name.c_str());
        return false;
    }
    m_state = MsWorking;
    LOG_INFO("Module[%s] start ok -> Working", m_name.c_str());
    return true;
}

void Module::stop()
{
    if (MsPause == m_state)
    {
        resume();
    }

    if (MsWorking == m_state)
    {
        LOG_DEBUG("Module[%s] stop ...", m_name.c_str());
        doStop();
        m_state = MsReady;
        LOG_INFO("Module[%s] stop ok -> Ready", m_name.c_str());
    }
}

bool Module::resume()
{
    if (MsPause != m_state)
    {
        LOG_WARN("Module[%s] resume skipped, state=%s", m_name.c_str(), stateName(m_state));
        return false;
    }
    if (!doResume())
    {
        LOG_ERROR("Module[%s] doResume failed", m_name.c_str());
        return false;
    }
    m_state = MsWorking;
    LOG_INFO("Module[%s] resume ok -> Working", m_name.c_str());
    return true;
}

bool Module::suspend()
{
    if (MsWorking != m_state)
    {
        LOG_WARN("Module[%s] suspend skipped, state=%s", m_name.c_str(), stateName(m_state));
        return false;
    }
    if (!doSuspend())
    {
        LOG_ERROR("Module[%s] doSuspend failed", m_name.c_str());
        return false;
    }
    m_state = MsPause;
    LOG_INFO("Module[%s] suspend ok -> Pause", m_name.c_str());
    return true;
}

bool Module::doStart()
{
    return true;
}

void Module::doStop() {}

bool Module::doResume()
{
    return doStart();
}

bool Module::doSuspend()
{
    doStop();
    return true;
}

const std::string& Module::getName()
{
    return m_name;
}

void Module::setName(const std::string& name)
{
    m_name = name;
}

} // namespace fw
} // namespace afl
