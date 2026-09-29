/**
 * @file   AppConfig.h
 * @brief  demo_app 应用配置（JSON）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/config/ConfigData.h"

#include <string>

namespace demo
{

/**
 * @brief 应用级配置：日志、心跳周期、自动退出次数
 *
 * 对应 conf/app.json 中键 "app"。
 */
struct AppConfig : public afl::config::ConfigData<AppConfig>
{
    std::string logLevel = "info";
    std::string logDir = "logs";
    double tickIntervalSec = 1.0;
    /** @brief 心跳触发次数上限；0 表示一直跑到 Ctrl+C */
    int maxTicks = 5;

    void writeToFile(afl::config::ConfigBlock& cb) override
    {
        cb["logLevel"] = logLevel;
        cb["logDir"] = logDir;
        cb["tickIntervalSec"] = tickIntervalSec;
        cb["maxTicks"] = maxTicks;
    }

    void readFromFile(const afl::config::ConfigBlock& cb) override
    {
        logLevel = cb.value("logLevel", std::string("info"));
        logDir = cb.value("logDir", std::string("logs"));
        tickIntervalSec = cb.value("tickIntervalSec", 1.0);
        maxTicks = cb.value("maxTicks", 5);
    }
};

} // namespace demo
