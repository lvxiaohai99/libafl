/**
 * @file   main.cpp
 * @brief  libafl 综合示例：配置 + 日志 + Module + EventLoop + Signal
 * @author libafl
 * @date   2026-09
 *
 * 运行:
 *   ./afl_demo_app [conf目录]
 * 默认 conf 目录为可执行文件旁的 conf/。
 * Ctrl+C / SIGTERM 优雅退出；maxTicks>0 时跑满次数后自动退出。
 */
#include "AppConfig.h"
#include "StatsModule.h"
#include "TickModule.h"

#include "afl/config/ConfigManager.h"
#include "afl/config/Configurable.h"
#include "afl/framework/EventLoopManager.h"
#include "afl/framework/ModuleManager.h"
#include "afl/log/Log.h"
#include "afl/net/SignalHandler.h"
#include "afl/concurrency/Thread.h"

#include <libgen.h>
#include <limits.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <memory>
#include <string>

namespace
{

std::string exeDir()
{
    char buf[PATH_MAX];
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0)
    {
        return ".";
    }
    buf[n] = '\0';
    char* dir = ::dirname(buf);
    return dir ? std::string(dir) : std::string(".");
}

std::string resolveConfDir(int argc, char* argv[])
{
    if (argc >= 2 && argv[1] && argv[1][0] != '\0')
    {
        return argv[1];
    }
    return exeDir() + "/conf";
}

/** @brief worker 周期定时器回调（供 std::bind 使用） */
void onWorkerRepeat(std::shared_ptr<afl::net::EventLoop> loop, std::shared_ptr<int> count,
                    std::shared_ptr<afl::net::TimerId> timerId, double intervalSec, int maxTimes)
{
    ++(*count);
    LOG_INFO("[Timer/worker] repeating #%d (every %.1fs, tid=%d)", *count, intervalSec,
             afl::concurrency::this_thread::tid());
    if (*count >= maxTimes)
    {
        loop->cancelTimer(*timerId);
        LOG_INFO("[Timer/worker] repeating cancelled after %d times", maxTimes);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    const std::string confDir = resolveConfDir(argc, argv);

    // ---------- 1. 配置 ----------
    afl::config::ConfigManager configMgr(confDir);
    afl::config::Configurable<demo::AppConfig> appCfg(configMgr, "app", "app.json");
    demo::AppConfig& conf = appCfg.getConfig();

    // ---------- 2. 日志：失败则回落控制台，不因此退出 ----------
    const std::string logDir = exeDir() + "/" + conf.logDir;
    if (!afl::log::setupAppLogger(logDir, "demo_app.log", conf.logLevel, conf.logLevel,
                                  afl::log::kDefaultMaxFileSizeBytes, afl::log::kDefaultMaxFiles,
                                  "demo_app"))
    {
        afl::log::setupConsoleLogger(conf.logLevel, "demo_app");
        LOG_WARN("setupAppLogger failed, using console logger");
    }

    LOG_INFO("======== afl demo_app start ========");
    LOG_INFO("confDir=%s interval=%.3f maxTicks=%d logLevel=%s (file: %s/demo_app.log)",
             confDir.c_str(), conf.tickIntervalSec, conf.maxTicks, conf.logLevel.c_str(),
             logDir.c_str());


    // ---------- 3. EventLoop：主循环 + 命名工作循环（两条） ----------
    afl::fw::EventLoopManager loopMgr;
    auto mainLoop = loopMgr.getEventLoop(); // 空名 = 主线程 loop

    // unique=true：同名始终拿到同一条专用线程上的 EventLoop
    if (loopMgr.createCustomAttributes("worker", true) != 0)
    {
        LOG_ERROR("createCustomAttributes(worker) failed");
        return 1;
    }
    auto workerLoop = loopMgr.getEventLoop("worker");
    // 必须一直持有 workerLoop，引用计数掉到 1 时工作线程会自行 quit
    LOG_INFO("EventLoops: main=%p worker=%p (same? %d)", mainLoop.get(), workerLoop.get(),
             mainLoop.get() == workerLoop.get());

    // ---------- 4. 模块注册（先构造 tick，避免 forEach 持锁时再 getModuleInstance 死锁） ----------
    afl::fw::ModuleManager modules;

    auto tick = std::make_shared<demo::TickModule>(mainLoop, conf.tickIntervalSec, conf.maxTicks,
                                                   [&]() { loopMgr.quit(); });
    modules.registerModule("tick", 10, [tick]() -> std::shared_ptr<afl::fw::Module> { return tick; });
    modules.registerModule("stats", 20, [tick]() -> std::shared_ptr<afl::fw::Module> {
        return std::make_shared<demo::StatsModule>(tick);
    });

    // ---------- 5. 生命周期：升序 init/start ----------
    bool ok = true;
    modules.forEach([&](std::shared_ptr<afl::fw::Module> m) {
        if (!ok)
        {
            return;
        }
        if (!m->init())
        {
            LOG_ERROR("module %s init failed", m->getName().c_str());
            ok = false;
            return;
        }
        if (!m->start())
        {
            LOG_ERROR("module %s start failed", m->getName().c_str());
            ok = false;
        }
    });
    if (!ok)
    {
        modules.forEachReverse([](std::shared_ptr<afl::fw::Module> m) {
            m->stop();
            m->deinit();
        });
        return 1;
    }

    // ---------- 6. 定时器示例（主 loop + worker loop 各挂一个） ----------
    // 6.1 主 loop：一次性
    const double oneShotDelaySec = 1.2;
    mainLoop->addTimer(
        [oneShotDelaySec] {
            LOG_INFO("[Timer/main] one-shot fired after %.1fs", oneShotDelaySec);
        },
        oneShotDelaySec, false);

    // 6.2 worker loop：周期性，触发若干次后 cancel（std::bind 写法）
    auto repeatCount = std::make_shared<int>(0);
    auto repeatId = std::make_shared<afl::net::TimerId>(0);
    const double repeatIntervalSec = 0.8;
    const int repeatMax = 3;
    *repeatId = workerLoop->addTimer(
        std::bind(&onWorkerRepeat, workerLoop, repeatCount, repeatId, repeatIntervalSec, repeatMax),
        repeatIntervalSec, true);

    // ---------- 7. Ctrl+C 优雅退出 ----------
    afl::net::SignalHandler signals(*mainLoop);
    auto quitOnSignal = [&](afl::net::SignalHandler::SigInfo&) {
        LOG_WARN("signal received, quitting...");
        loopMgr.quit();
    };
    signals.addSignal(SIGINT, quitOnSignal);
    signals.addSignal(SIGTERM, quitOnSignal);

    LOG_INFO("event loop running (Ctrl+C to quit)...");
    loopMgr.run();

    // ---------- 8. 降序 stop/deinit ----------
    modules.forEachReverse([](std::shared_ptr<afl::fw::Module> m) {
        m->stop();
        m->deinit();
    });

    workerLoop.reset();

    auto stats = modules.getModuleInstance<demo::StatsModule>("stats");
    if (stats)
    {
        LOG_INFO("stats received ticks=%d", stats->receivedCount());
    }

    appCfg.saveConfig();
    LOG_INFO("======== afl demo_app exit ========");
    return 0;
}
