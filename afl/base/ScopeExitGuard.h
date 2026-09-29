/**
 * @file   ScopeExitGuard.h
 * @brief  RAII 类，用于资源释放与清理
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <functional>

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback)                                                                    \
    afl::base::ScopeExitGuard SCOPEGUARD_LINENAME(EXIT, __LINE__)(callback)

namespace afl
{
namespace base
{
class ScopeExitGuard
{
public:
    explicit ScopeExitGuard(std::function<void()> onExitCallback)
        : m_onExitCb(onExitCallback), m_dismissed(false)
    {
    }

    ScopeExitGuard(ScopeExitGuard&& rhs)
        : m_onExitCb(std::move(rhs.m_onExitCb)), m_dismissed(rhs.m_dismissed)
    {
    }

    ~ScopeExitGuard()
    {
        if (!m_dismissed)
        {
            m_onExitCb();
        }
    }

    void dismiss() { m_dismissed = true; }

private:
    std::function<void()> m_onExitCb;
    bool m_dismissed;

private:
    ScopeExitGuard(ScopeExitGuard const&);
    ScopeExitGuard& operator=(ScopeExitGuard const&);
};

template <typename F>
ScopeExitGuard makeScopeExitGuard(F&& f)
{
    return ScopeExitGuard(std::forward<F>(f));
}

} // namespace base
} // namespace afl
