# libafl — 通用 C++11 工具库（Linux）v1.1.0

libafl（Application Foundation Library）面向 Linux 的通用 C++11 基础库，
按功能模块划分，可直接集成到任意 C++ 项目。

| 项 | 说明 |
|----|------|
| 语言 | C++11（兼容 gcc 4.9 / gcc-linaro-4.9.4） |
| 平台 | Linux（epoll / signalfd / timerfd） |
| 构建 | CMake 3.10+ |
| 测试 | googletest 1.12.1（内置） |
| 文档 | Doxygen HTML（`./build.sh --doc`） |
| IDE | `compile_commands.json`（`./build.sh` 生成，含 examples / asn1 codec demo） |

## 目录结构

```
libafl/
├── afl/                  # 库本体
│   ├── base/             # NonCopy、Singleton、ObjectPool、Any、Closure、Signal …
│   ├── string/           # StringUtil、StringPiece          (afl::str)
│   ├── crypto/           # Md5、Sha1、Crc、Base64、AesHelper、StringHash
│   ├── time/             # Date、DateTime、TimeStamp、StopWatch …
│   ├── file/             # File、FileUtil                   (afl::file)
│   ├── log/              # Log、LoggerManager、PathAnalyzer (afl::log)
│   ├── config/           # ConfigManager、ConfigFile、Configurable
│   ├── concurrency/      # Thread、ThreadPool、队列、RWMutex、ConcurrentHashMap
│   ├── process/          # Daemonize、ProcessUtil、SingletonProgram
│   ├── net/              # EventLoop、Tcp*、http/、websocket/、SslHelper
│   └── framework/        # Module、ModuleManager …          (afl::fw)
│   └── asn1/             # Asn1Cpp（asn1c RAII）+ Asn1Json（结构转 JSON），头文件，需配合 asn1c 生成代码
├── examples/             # 综合示例（demo_app）
├── thirdparty/           # spdlog / nlohmann / googletest / asn1c（见各目录 README）
├── test/                 # gtest
├── scripts/              # gen_coverage_report.sh 等
├── cmake/                # 交叉工具链 + aflConfig.cmake.in
├── CMakeLists.txt
├── build.sh
└── Doxyfile
```

## 命名规范（v1.1 破坏性变更）

- 文件名 / 类 / 枚举：PascalCase（`ConfigManager.h`、`PathAnalyzer`、`AesHelper`）
- 函数 / 变量：camelCase（`tryLock`、`joinAll`、`encrypt`、`saveConfig`）
- 成员变量：`m_` 前缀（`m_loop`、`m_socket`）
- 头文件：统一 `#pragma once`
- 宏：`AFL_` 前缀或模块约定日志宏
- **不保留**旧拼写 / snake_case / 旧别名（`saveConfigure`、`SSL_Helper`、`CallBack` 等已删除）

## 快速开始

```bash
./build.sh              # 本机构建 + ctest + 安装到 ./install
./build.sh --shared     # 动态库
./build.sh --arm        # arm 交叉（不跑测试，装到 ./install-arm）
./build.sh --coverage   # 一键生成单元测试覆盖率报告（行 + 分支）
./build.sh --demo       # 构建、安装并运行综合示例（用 install/bin）
./build.sh --prefix=/tmp/afl-stage  # 指定安装目录
./build.sh --no-install # 只编译不安装
./build.sh --doc        # Doxygen → docs/html
./build.sh --clean
```

默认安装布局（本地测试用，不写系统目录）：

```
install/
├── bin/afl_demo_app
├── bin/conf/app.json
├── include/afl/ ...
├── include/spdlog/ ...
├── include/nlohmann/ ...
└── lib/libafl.a（或 .so）+ cmake/afl/
```

CMake 选项：

| 选项 | 默认 | 说明 |
|------|------|------|
| `AFL_BUILD_SHARED` | OFF | 动态库 |
| `AFL_ENABLE_SSL` | ON | `SslHelper`（无 OpenSSL 则自动关闭） |
| `AFL_BUILD_TESTS` | ON | 单元测试 |
| `AFL_BUILD_EXAMPLES` | ON | 示例程序（`afl_demo_app`） |
| `AFL_ENABLE_COVERAGE` | OFF | gcov 行/分支覆盖率插桩 |

## 集成到你的项目

### 方式一：add_subdirectory

```cmake
set(AFL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/libafl)
target_link_libraries(your_target PRIVATE afl::afl)  # 或 afl
```

### 方式二：安装后 find_package

```bash
./build.sh                    # 已安装到 ./install
# 或指定目录：
./build.sh --prefix=/tmp/afl
cmake --install build --prefix /usr/local   # 仍可装系统
```

```cmake
find_package(afl REQUIRED)
target_link_libraries(your_target PRIVATE afl::afl)
```

安装内容包含 `afl/` 头文件以及随包装的 **spdlog / nlohmann** 头，开箱可用。

## 使用示例

### 配置（ConfigManager / Configurable）

```cpp
#include "afl/config/ConfigManager.h"
#include "afl/config/Configurable.h"
#include "afl/config/ConfigData.h"

struct AppConf : public afl::config::ConfigData<AppConf> {
    int port = 8080;
    void writeToFile(afl::config::ConfigBlock& cb) override { cb["port"] = port; }
    void readFromFile(const afl::config::ConfigBlock& cb) override {
        port = cb.value("port", 8080);
    }
};

afl::config::ConfigManager mgr("./conf");
afl::config::Configurable<AppConf> cfg(mgr, "app", "app.json");
cfg.getConfig().port = 9000;
cfg.saveConfig();           // 写回 app.json
mgr.reloadConfig("./conf/app.json");  // 热重载
```

### 信号/槽（类 Qt / nod）

```cpp
#include "afl/base/Signal.h"

afl::base::Signal<void(int, const char*)> sig;
afl::base::ScopedConnection conn = sig.connect([](int id, const char* msg) {
    // 槽回调
});
sig(1, "hello");   // 发射
// conn 析构时自动断开
```

对应 RSU `MetaEvent::m_Signal`（原 `nod::signal`）的基础能力；线程安全默认开启。

### 日志

格式固定为：

```text
[2021-01-03 17:11:41.843|I|mt_data|config_client.cpp(232)]: message
```

（时间 | 级别字母 T/D/I/W/E/C | logger 名 | 源文件(行号)）

未调用 `setup*` 时，`LOG_*` 自动回落控制台，不会拖垮启动；
文件目录创建失败时 `setupAppLogger` / `setupFileLogger` 也会退回控制台。

```cpp
#include "afl/log/Log.h"

int main() {
    // 推荐：控制台 + 滚动文件（默认终端 warn、文件 info、10MB×6）
    afl::log::setupAppLogger("logs", "app.log", "info", "warn",
                             afl::log::kDefaultMaxFileSizeBytes, afl::log::kDefaultMaxFiles,
                             "mt_data");
    // 自定义单文件大小与滚动个数：
    // afl::log::setupAppLogger("logs", "app.log", "info", "warn", 20 * 1024 * 1024, 10);

    LOG_INFO("server started, port=%d", 8080);
    return 0;
}
```

### 字符串 / 加密

```cpp
#include "afl/string/StringUtil.h"
#include "afl/crypto/Base64.h"
#include "afl/crypto/AesHelper.h"

using namespace afl::str;
using namespace afl::crypto;
auto parts = split("a,b,c", ',');
std::string b64 = base64Encode("hello");
std::string cipher = AesHelper::encrypt("hello", "0123456789abcdef");
```

### 并发

```cpp
#include "afl/concurrency/ThreadPool.h"
#include "afl/concurrency/BlockingQueue.h"
#include "afl/concurrency/ConcurrentHashMap.h"

afl::concurrency::ThreadPool pool("workers");
pool.start(4);
pool.run([] { /* task */ });

afl::concurrency::BlockingQueue<int> queue;
queue.push(1);
int v = 0;
queue.pop(v);

afl::concurrency::ConcurrentHashMap<std::string, int> map;
map.put("k", 1);
auto snap = map.toMap();
```

### 综合示例 demo_app

把配置、日志、Module 生命周期、EventLoop 定时器、Signal 槽、SIGINT 退出串在一起：

```bash
./build.sh --demo
# 或手动：
./build/bin/afl_demo_app            # 读可执行文件旁 conf/app.json
./build/bin/afl_demo_app /path/conf # 指定配置目录
```

流程简述：读 `app.json` → 控制台+文件日志 → 注册 `TickModule`/`StatsModule` →
定时发射 `Signal` → Stats 订阅打印 → 满 `maxTicks` 或 Ctrl+C 后按反序 stop/deinit。

## 代码风格

仓库根目录提供 `.clang-format`（Allman 大括号、4 空格、禁止 Tab）。

```bash
# 对本库源码做格式化（不含 thirdparty）
find afl test -type f \( -name '*.h' -o -name '*.cpp' -o -name '*.c' \) -print0 \
  | xargs -0 clang-format -i
```

约定摘要：

| 项 | 规则 |
|----|------|
| 头文件 | `#pragma once` + Doxygen `@file/@brief` |
| 成员 | `m_camelCase`（类型别名/嵌套类不加 `m_`） |
| 类型 | 头文件声明优先 `std::string` |
| 缩进 | 4 空格，无 Tab |

### 注释规范（统一中文）

| 项 | 规则 |
|----|------|
| 语言 | **一律中文**；禁止整句中英混写 |
| 专有名词 | 可保留缩写：TCP / HTTP / SSL / JSON / RAII / fd / pid / epoll 等 |
| 文件头 | `@file` / `@brief`（中文）/ `@author libafl` / `@date 2026-09` |
| 公开 API | 类与关键函数补 `@brief`，必要时 `@param` / `@return`（中文） |
| 第三方许可 | 许可证原文（如 Google BSD、RSA MD5）**保留英文不翻译** |
| 禁止 | 旧 `@copyright`、Change History 横幅、`TODO Auto-generated`、乱码/截断中文 |

## 测试与文档

```bash
./build.sh
cd build && ctest --output-on-failure

./build.sh --doc
# 打开 docs/html/index.html
```

当前约 **77** 个用例，覆盖各模块核心路径；`Daemonize` / `MasterWorkerProcess` 等
涉及 fork 的 API 不做破坏性单测。

主机 `./build.sh` 与 `./build.sh --arm` 均应通过。

### 一键单元测试覆盖率报告（行 + 分支）

依赖：`cmake`、`g++`、`gcov`、`lcov`、`genhtml`（Ubuntu：`sudo apt-get install -y lcov`）。

```bash
./build.sh --coverage
# 或
./scripts/gen_coverage_report.sh
```

产物目录 `coverage-report/`：

| 文件 | 说明 |
|------|------|
| `html/index.html` | 可浏览的 HTML 报告（含行/分支着色） |
| `coverage.info` | lcov 原始数据 |
| `summary.txt` | 文本摘要 |

近期一次本机结果示例（仅统计 `afl/`，已剔除 thirdparty/test）：

- 行覆盖率约 **66%**
- 函数覆盖率约 **76%**
- 分支覆盖率约 **38%**

可选门槛（未达则脚本失败，便于 CI）：

```bash
MIN_LINE_COVERAGE=50 MIN_BRANCH_COVERAGE=20 ./scripts/gen_coverage_report.sh
```

#### 其它 CMake 项目如何借鉴

1. 拷贝 `scripts/gen_coverage_report.sh`
2. 工程增加类似 `XXX_ENABLE_COVERAGE`：编译加 `--coverage -O0 -g`，链接加 `--coverage`
3. 按需设置环境变量：

```bash
COVERAGE_SRC_ROOT=src \
COVERAGE_CMAKE_ARGS="-DMYLIB_ENABLE_COVERAGE=ON -DBUILD_TESTING=ON" \
COVERAGE_EXCLUDE="*/generated/*" \
./scripts/gen_coverage_report.sh
```

脚本会自动：干净构建 → 跑 `ctest` → `lcov` 采集（含分支）→ `genhtml` 出报告。

## 交叉编译

```bash
./build.sh --arm
# 产物：build-arm/libafl.a
```

可覆盖工具链根：  
`cmake -B build-arm -DAFL_TOOLCHAIN_DIR=/path/to/toolchain -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-linux-gnueabi.cmake .`

## 版本说明（1.1.0 清理要点）

- 公开 API 统一 camelCase + `#pragma once` + Doxygen
- **config**：`saveConfig` / `reloadConfig`；`Configurable` 按配置键保存（不再误用句柄当字符串）；根目录尾斜杠规范化
- **ConcurrentHashMap**：修复析构泄漏；导出接口改为 `toMap()`
- **SslHelper**：类名 camelCase；去掉 `exit()` / 宏 abort；缺证书返回 `false`
- **删除死代码**：`Array` / `MathUtil` / `Random` / `Range` / `StlUtil` / 空壳 `HighPrecisionTime` /
  无人引用的 `JenkinsHash` / `Murmur3Hash`
- 删除旧别名：`saveConfigure`、`UpdateCallBack` 等
- **全库格式**：`.clang-format` + 全量格式化；process/time/string/file/log/framework/net 关键头命名统一

## 第三方工具：asn1c

ASN.1 → C 代码生成器，源码在 `thirdparty/asn1c-master/`。与 libafl 本体分开构建：

```bash
cd thirdparty/asn1c-master
./build.sh                 # 安装到该目录下 install/
cd demo && ./run_demo.sh   # CCSA MessageFrame：asn_src → asn_out + libasn1c
cd codec_demo && ./run.sh  # C++ UPER/XER 编解码（用 afl/asn1/Asn1Cpp.h）
# 详细用法见 thirdparty/asn1c-master/README.zh.md
```

头文件 `afl/asn1/Asn1Cpp.h`：对 asn1c 生成类型做 RAII（`encode`/`decode`/`encodeToFile`/`decodeFromFile`/`encodeHex`/`decodeHex`、`dump()` 打印完整结构、`toJson()` 输出 JSON），支持 UPER、XER、BER、OER。JSON 由 `afl/asn1/Asn1Json.h` 按类型描述符生成，仅用于显示，不能反向解码。

hex 工具 `afl/string/Hex2String.h`：`toHexDump` / `parseHexBytes` 为二进制安全版本（可含 0x00，解析容忍空格、`0x`、`:`、`,`）；旧的 `encodeToHexString` / `decodeHexString` 按 C 字符串处理，遇 0x00 截断，不要用于编码数据。

内存：根节点析构整棵释放；中途清空/删除子结构只能用 `Asn1List::clear/remove`、`resetField`、`freeField`（按类型描述符递归释放），禁止 `asn_sequence_empty` / 裸 `free`。规则详见 `thirdparty/asn1c-master/demo/README.md`「内存释放规则」。

## 已知局限与后续改进

- `MasterWorkerProcess` / `Daemonize` 涉及 fork，单元测试仅覆盖非破坏性 process API
- `SslHelper` 未做真实 TLS 握手集成测试（需证书与对端）
- 部分 net 日志仍用 `%x` 打印指针，arm 交叉会有 format 告警，不影响链接
- `FileUtil` 不再向 `afl::file` 注入 `using namespace FileUtil`，请写 `FileUtil::xxx`
- `afl::base::Signal` 已对齐 nod 核心 API；RSU `MetaEvent` 迁移时对照：
  `slot_count`→`slotCount`，`disconnect_all_slots`→`disconnectAllSlots`，
  `nod::connection`→`afl::base::Connection`；尚未移植 nod 的 accumulate/adapter
- 综合示例见 `examples/demo_app`（`./build.sh --demo`）
- asn1c 为宿主机工具；生成的 `.c` 再交叉编进 OBU/RSU，详见 `thirdparty/asn1c-master/README.zh.md`
