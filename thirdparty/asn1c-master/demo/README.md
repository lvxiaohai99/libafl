# asn1c demo：CCSA 2020 MessageFrame（协议与 libasn1c 分离）

用本仓库安装的 `asn1c`，把规格与运行时骨架**分开**存放，对齐 RSU 工程：

| 本 demo | RSU 对应 |
|---------|----------|
| `asn_src/` | `libccsa_2020/MessageFrame/asn1/` |
| `asn_out/` | `libccsa_2020/MessageFrame/src/`（仅协议生成代码） |
| `libasn1c/` | `libasn1c/`（asn1c 骨架 / runtime） |

## 目录

```
demo/
├── asn_src/          # ASN.1 源（.asn）
├── asn_out/          # 协议生成的 .c/.h（-R，不含骨架）
├── libasn1c/         # 骨架 runtime（从 install/share/asn1c 同步）
├── codec_demo/       # C++ 编解码示例（afl::asn1::Asn1Ptr）
│   ├── main.cpp
│   ├── run.sh
│   └── samples/      # 运行后生成 bsm.uper / bsm.xml
├── run_demo.sh
└── README.md
```

源文件来自：`rsu/new4layer_rsu_3a31/libccsa_2020/MessageFrame/asn1/*.asn`

## 为什么要分开？

默认直接跑 `asn1c … -D out` 会把骨架（`INTEGER.c`、`ber_decoder.c` 等）**拷进同一目录**，和协议类型混在一起。

加上 **`-R`**（Restrict：只生成表/协议代码，不拷贝支撑代码）后：

- `asn_out/` 只剩 MessageFrame / BSM / Map … 等协议文件
- `libasn1c/` 单独放 runtime，可做成共享库（如工程里的 `libasn1c.so`），多个协议复用

`-R` **不会**生成 `libasn1c`：骨架是脚本从 `install/share/asn1c` **拷贝**过来的。

## 运行：生成协议代码

```bash
cd .. && ./build.sh          # 若尚未安装 asn1c
cd demo
./run_demo.sh                # 生成 asn_out + 同步 libasn1c
./run_demo.sh --clean
```

## 运行：C++ 编解码 demo（UPER / XER）

依赖 `afl/asn1/Asn1Cpp.h`。默认对 **BSM / RSM / SPAT / MAP / RSI** 全做 roundtrip：

| 消息 | 覆盖点 |
|------|--------|
| BSM | 基础 SEQUENCE、OPTIONAL 有/无 |
| RSM | `SEQUENCE OF` 列表、多 OPTIONAL 混用、PositionOffset CHOICE |
| SPAT | 嵌套列表、BIT STRING、TimeChangeDetails CHOICE |
| MAP | NodeList、name/region/elevation OPTIONAL 混用 |
| RSI | RTE/RTS 双列表、Description CHOICE、ReferenceLanes BIT STRING |
| 列表所有权 | 装满 16 条 RTS → `clear()` → 复用头部再装、`remove`/`pick`+`freeElement`、`resetField`/`freeField` |

用 `valgrind --leak-check=full build/bin/afl_asn1_codec_demo` 可验证 0 泄漏（分配次数 = 释放次数）。

```bash
cd demo/codec_demo
./run.sh                              # 经 libafl CMake 编译（写入 compile_commands，可 IDE 跳转）
./run.sh encode-sample -o samples
./run.sh decode --uper samples/rsi.uper
./run.sh decode --xer  samples/rsi.xml
./run.sh --clean
```

IDE 跳转：`codec_demo` 已挂到 libafl 主 CMake；在 `obu/libafl` 下执行过 `./build.sh`（或 `./run.sh`）后，点击 `afl::asn1::Asn1List` / `Asn1Cpp.h` 即可转到定义。若无效，重载 clangd 窗口。

核心 API：

```cpp
#include "MessageFrame.h"
#include "afl/asn1/Asn1Cpp.h"
AFL_DECLARE_ASN1_TYPE(MessageFrame)

afl::asn1::MessageFramePtr mf;
mf.encodeToFile<afl::asn1::B_UPER>("a.uper");
mf.encodeToFile<afl::asn1::B_XER>("a.xml");
mf.decodeFromFile<afl::asn1::B_UPER>("a.uper");
mf.decodeFromFile<afl::asn1::B_XER>("a.xml");
```

### 内存释放规则

根节点 `XxxPtr` 析构时用 `ASN_STRUCT_FREE` 整棵释放，平时不用手动 free。只有「中途删除 / 清空子结构」时要注意：

| 要做的事 | 正确写法 | 禁止写法（会泄漏） |
|----------|----------|--------------------|
| 清空整张列表后复用 | `Asn1List<RTSList_t>(l).clear()` 或 `afl::asn1::resetField(*rsi->rtss)` | `asn_sequence_empty` + `free` |
| 删除一个元素 | `list.remove(i)` | `asn_sequence_del(&l.list, i, 1)` |
| 取出元素另用 | `list.pick(i)`，用完 `list.freeElement(e)` 或挂到别处 | 取出后不管 |
| 删除 OPTIONAL 子结构 | `afl::asn1::freeField(rsi->rtes)` | `free(rsi->rtes)` |

`clear`/`remove`/`freeElement`/`resetField(x)`/`freeField(p)` 需要对该类型写 `AFL_DECLARE_ASN1_TYPE(RTSList)`，漏写会编译报错（不会静默泄漏）。原因：asn1c 生成的列表 `list.free` 回调为空，`asn_sequence_empty` 只释放指针数组，元素及其子树全部泄漏；必须按类型描述符递归释放。

## 使用的 asn1c 参数

```text
-R -no-gen-example -findirect-choice -fincludes-quoted
-fline-refs -funnamed-unions -pdu=all -D asn_out
```

## 业务工程怎么编

头文件搜索路径要同时包含两边：

```bash
g++ -Iasn_out -Ilibasn1c -I<path-to-libafl> …
```
