# demo_app — libafl 综合示例

演示如何把 AFL 拼成一个可运行的小应用。

## 用到了什么

| 能力 | AFL 组件 | 本示例里干什么 |
|------|----------|----------------|
| 配置 | `ConfigManager` / `Configurable` | 读 `conf/app.json` |
| 日志 | spdlog 双 sink | `setupAppLogger` 一行初始化 |
| 模块 | `Module` / `ModuleManager` | Tick + Stats 生命周期 |
| 事件循环 | `EventLoopManager` | 主 loop + 命名 `worker` loop；定时器分挂两边 |
| 信号槽 | `afl::base::Signal` | Tick 发射，Stats 订阅 |
| 系统信号 | `SignalHandler` | Ctrl+C / SIGTERM 退出 |

## 怎么跑

```bash
# 在 libafl 根目录
./build.sh --demo

# 或手动
./build/bin/afl_demo_app
./build/bin/afl_demo_app /path/to/conf
```

改 `conf/app.json` 里的 `tickIntervalSec` / `maxTicks`（`0` = 一直跑到 Ctrl+C）后重跑即可。

## 目录

```
demo_app/
├── main.cpp          # 组装：配置→日志→模块→loop
├── AppConfig.h       # JSON 配置结构
├── TickModule.h      # 定时器 + Signal 发射
├── StatsModule.h     # 订阅 Signal
├── conf/app.json
└── CMakeLists.txt
```
