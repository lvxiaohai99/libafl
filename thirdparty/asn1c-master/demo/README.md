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
| BSM | 基础 SEQUENCE、OPTIONAL 有/无、可扩展变长 BIT STRING（safetyExt.events） |
| RSM | `SEQUENCE OF` 列表、多 OPTIONAL 混用、PositionOffset CHOICE |
| SPAT | 嵌套列表、BIT STRING、TimeChangeDetails CHOICE |
| MAP | NodeList、name/region/elevation OPTIONAL 混用 |
| RSI | RTE/RTS 双列表、Description CHOICE、ReferenceLanes BIT STRING |
| 列表所有权 | 装满 16 条 RTS → `clear()` → 复用头部再装、`remove`/`pick`+`freeElement`、`resetField`/`freeField` |
| JSON 错误输入 | 非法 JSON、未知成员、缺必选成员、枚举名错、类型不符、整数越界、CHOICE 多分支都被拒绝并给出路径 |

用 `valgrind --leak-check=full build/bin/afl_asn1_codec_demo` 可验证 0 泄漏（分配次数 = 释放次数）。

```bash
cd demo/codec_demo
./run.sh                              # 经 libafl CMake 编译（写入 compile_commands，可 IDE 跳转）
./run.sh encode-sample -o samples     # 写 .uper / .xml / .hex / .json
./run.sh decode --uper samples/rsi.uper
./run.sh decode --xer  samples/rsi.xml
./run.sh decode --json samples/rsi.json    # JSON 输入 → 结构 → 重新 UPER 编码并打印 hex
./run.sh decode --hex  "00 00 E8 88 ..."   # 也接受 0000E888…、0x00,0x00,…、00:00:…
./run.sh decode --hex-file samples/rsi.hex
./run.sh decode --hex-file samples/rsi.hex --format json   # JSON 显示；both = asn_print + JSON
./run.sh --clean
```

JSON 规则（`afl/asn1/Asn1Json.h`；asn1c 0.9.29 没有 JER，这里按类型描述符实现，格式对齐 JER / X.697，**可双向**：`toJson` 输出，`fromJson` 读回，读回后 UPER 字节与原来一致）：

| ASN.1 类型 | JSON |
|------------|------|
| SEQUENCE / SET | 对象；未填的 OPTIONAL 不出现 |
| CHOICE | `{"分支名": 值}`，如顶层 `{"bsmFrame": {...}}` |
| 开放类型（`&Type`） | 直接写内层值；读回时按同级 id 字段（`type_selector`）选类型 |
| SEQUENCE OF | 数组 |
| INTEGER（native） | 数字 |
| ENUMERATED | 枚举名，如 `"forwardGears"`（输入也接受数字） |
| BOOLEAN / NULL | `true`/`false` / `null` |
| IA5String / UTF8String 等 | 字符串 |
| OCTET STRING | 大写 hex，如 `"id": "44454D4F30303031"` |
| 定长 BIT STRING（`SIZE(n)`，不可扩展） | hex，如 `"referenceLanes": "6000"`（16 位） |
| 变长 / 可扩展 BIT STRING | `{"value": hex, "length": 位数}`，如 `"events": {"value": "A000", "length": 13}` |
| 其它（大整数、REAL 等） | 仅输出：asn1c 打印文本；不支持读回 |

JSON 输入校验：未知成员、缺必选成员、类型不符、整数超出 C 类型范围、枚举名不存在都会失败，错误信息带路径，如
`rsiFrame.rtss[0].referenceLinks[0].referenceLanes: expected hex string`。失败时目标对象保持原样。
`fromJson` 只保证「能装进结构」，业务取值范围（如 `rtsId 0..255`）由 `check()` / 编码时检查。

程序本体在 `obu/libafl/build/bin/afl_asn1_codec_demo`；`codec_demo/build/asn1_codec_demo` 是 `run.sh` 维护的软链接，始终指向最新构建。`-h` 里看不到 `--hex` / `--format` 说明运行的是旧程序，重新执行 `./run.sh` 即可。

输出约定：每次编码、解码后都打印 `asn_print`（`asn_fprint`）完整结构；UPER 编码后与 UPER/hex 输入解码前打印 hex 字节（每行 16 字节）。roundtrip 对每类消息依次验证 UPER 文件、XER 文件、UPER hex、JSON 文件四条解码路径，每条都要求重新编码后 UPER 字节完全一致。hex 文本非法（奇数位、非 hex 字符）或解码失败时返回码为 1。

IDE 跳转：`codec_demo` 已挂到 libafl 主 CMake；`afl/asn1/Asn1Ide.cpp` 保证单独打开 `Asn1Cpp.h` / `Asn1Json.h` 时也能解析（这两个头要求先有 asn1c 生成头）。在 `obu/libafl` 下执行过 `./build.sh`（或 `./run.sh`）后可跳转。若仍无效：命令面板执行 “clangd: Restart language server”，并确认工作区能看到 `obu/libafl/compile_commands.json`。

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

std::cout << mf.dump();                          // asn_fprint 完整结构（不截断）
std::cout << mf.toJson();                        // JSON（缩进 2）；toJson(0) 单行，适合写日志
std::string hex = mf.encodeHex<afl::asn1::B_UPER>(); // "00 00 E8 88 ..."
mf.decodeHex<afl::asn1::B_UPER>(hex);            // 格式同 afl::str::parseHexBytes

std::string err;
if (!mf.fromJson(jsonText, &err))                // 或 mf.fromJsonFile("a.json", &err)
    std::cerr << err << std::endl;               // 失败时 mf 不变
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
