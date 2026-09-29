/**
 * @file   Module.h
 * @brief  业务模块生命周期（init/start/suspend/stop）
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/framework/EventLoopManager.h"
#include "afl/net/EventLoop.h"
#include "afl/base/NonCopy.h"

namespace afl
{
namespace fw
{
/**
 * @brief 模块状态机
 * @details 状态迁移示意：
 *                init            start                suspend
 *  in-place   <------->  ready  <------->   working  <------->  pause
 *               deinit            stop                 resume
 *
 *  - in-place：模块已注册，尚未与其它模块建立依赖关系
 *  - ready：    已与其它模块关联，可启动业务
 *  - working：  业务正在运行
 *  - pause：    业务暂停；默认 resume 后回到 ready
 */
class Module : public afl::base::NonCopy
{
protected:
    Module() = default;

public:
    /** @brief 模块生命周期状态 */
    enum ModuleState
    {
        MsInPlace,
        MsReady,
        MsWorking,
        MsPause,
    };

    virtual ~Module() = default;

    /** @brief 初始化模块（in-place → ready） */
    bool init();
    /** @brief 反初始化模块（ready → in-place） */
    void deinit();

    /** @brief 启动业务（ready → working） */
    bool start();
    /** @brief 停止业务（working → ready） */
    void stop();

    /** @brief 从暂停恢复（pause → working） */
    bool resume();
    /** @brief 暂停业务（working → pause） */
    bool suspend();

    /** @brief 获取模块注册名 */
    const std::string& getName();

private:
    virtual bool doInit() = 0;
    virtual void doDeinit() = 0;

    virtual bool doStart();
    virtual void doStop();

    virtual bool doResume();
    virtual bool doSuspend();

    void setName(const std::string& name);

private:
    std::string m_name;
    ModuleState m_state = MsInPlace;
    friend class ModuleManager;
};

} // namespace fw
} // namespace afl
