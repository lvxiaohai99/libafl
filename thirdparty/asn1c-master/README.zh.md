# asn1c 使用说明（本仓库封装）

本目录是 [vlm/asn1c](https://github.com/vlm/asn1c)（ASN.1 → C 代码生成器）的源码树。
libafl / V2X 工程用它把 `.asn1` 规格编成可编解码的 `.c` / `.h`（支持 BER/DER、OER、UPER、XER）。

上游英文说明见同目录 [README.md](README.md)、[INSTALL.md](INSTALL.md)。
本文只写：**怎么编、装到哪、日常怎么用**。

---

## 1. 一键构建与安装

依赖（Ubuntu/Debian）：

```bash
sudo apt-get install -y build-essential autoconf automake libtool bison flex
```

在本目录执行：

```bash
chmod +x ./build.sh
./build.sh                 # 编译并安装到 ./install
./build.sh --check         # 额外跑 make check（慢）
./build.sh --prefix=/path  # 改安装目录
./build.sh --no-install    # 只编译
./build.sh --clean         # 清 build/ 与 install/
```

默认安装布局（不写系统目录）：

```
install/
├── bin/asn1c          # 主程序：ASN.1 → C
├── bin/unber          # BER 调试：二进制 → 可读树
├── bin/enber          # BER 调试：可读树 → 二进制
├── share/asn1c/       # 骨架代码（编生成代码时会自动引用）
├── share/doc/asn1c/
└── share/man/man1/
```

把工具加入当前 shell：

```bash
export PATH="$(pwd)/install/bin:$PATH"
asn1c -h
```

> **注意**：asn1c 是宿主机工具。交叉编译 ARM 时，仍在 x86 上跑 `asn1c` 生成代码，再把生成的 `.c` 用 ARM 工具链编译进 OBU/RSU。

---

## 1.1 Demo：CCSA 2020 MessageFrame（asn_src → asn_out，与 libasn1c 分开）

已把 RSU 工程里的 CCSA 2020 ASN.1 拷到 `demo/asn_src/`。生成时用 **`-R`**，协议代码和骨架 runtime **分目录**：

```bash
./build.sh                 # 若尚未安装 asn1c
cd demo && ./run_demo.sh
```

| 目录 | 内容 | 对应 RSU |
|------|------|----------|
| `demo/asn_src/` | `.asn` 规格（27 个） | `MessageFrame/asn1/` |
| `demo/asn_out/` | **仅**协议生成的 `.c/.h` | `MessageFrame/src/` |
| `demo/libasn1c/` | asn1c 骨架 runtime | `libasn1c/` |

关键点：加 **`-R`** 后 asn1c **不再**把 `INTEGER.c` / `ber_decoder.c` 等拷进输出目录；`run_demo.sh` 再把骨架同步到 `libasn1c/`。

```text
asn1c -R -no-gen-example -findirect-choice -fincludes-quoted \
      -fline-refs -funnamed-unions -pdu=all -D asn_out  asn_src/*.asn
```

编译时两个 `-I` 都要：

```bash
gcc -Iasn_out -Ilibasn1c …
```

更多说明见 [demo/README.md](demo/README.md)。

### C++ 封装与编解码 demo

RSU 里的 `asn1cpp.hpp` **有实际价值**（RAII + UPER/XER 编解码），已整理进：

`afl/asn1/Asn1Cpp.h` → 命名空间 `afl::asn1`（新接口，不兼容旧 `asn1cpp` 命名）。

编解码示例（BSM/RSM/SPAT/MAP/RSI，UPER+XER）：

```bash
cd demo/codec_demo
./run.sh                         # 五类消息全量 roundtrip
./run.sh decode --uper samples/rsi.uper
./run.sh decode --xer  samples/rsi.xml
```

---

## 2. 基本用法

### 2.1 编译一个 ASN.1 模块

```bash
mkdir -p /tmp/asn-out && cd /tmp/asn-out
asn1c /path/to/module.asn1
# 当前目录会生成大量 .c / .h（每个 ASN.1 类型一份）+ 骨架文件
```

多个互相依赖的模块必须一次全部传入：

```bash
asn1c module1.asn1 module2.asn1
```

### 2.2 推荐命令（V2X / 既有工程常用）

工程里常见写法：

```bash
mkdir -p out1
asn1c -no-gen-example \
      -findirect-choice \
      -fincludes-quoted \
      -fline-refs \
      -funnamed-unions \
      -pdu=all \
      -D out1 \
      ModuleA.asn1 ModuleB.asn1
```

逐项说明：

| 参数 | 作用 |
|------|------|
| `-no-gen-example` | 不生成示例程序 `converter-example.c`（业务库不需要） |
| `-findirect-choice` | `CHOICE` 成员改成指针间接引用，避免结构体过大 / 循环依赖；可配合 `-fno-include-deps` |
| `-fincludes-quoted` | `#include "xxx.h"` 用双引号，而不是 `<xxx.h>`，方便本地头文件搜索路径 |
| `-fline-refs` | 生成代码注释里带 ASN.1 源文件行号，方便对照规格排错 |
| `-funnamed-unions` | 结构体里用匿名 `union`，访问 CHOICE/可选字段更短（`s.field` 而不是 `s.u.field`） |
| `-pdu=all` | 把模块里**所有类型**都登记进 PDU 表（也可 `-pdu=auto` 只收顶层，或 `-pdu=MessageFrame` 指定类型） |
| `-D out1` | 生成文件写到目录 `out1/`（目录需已存在） |
| `-R` | **不拷贝骨架**；协议代码与 `libasn1c` 分开放时必加 |

可按需再加：

- `-fcompound-names`：大规格、多模块易重名时建议加上
- `-fno-include-deps`：和 `-findirect-choice` 一起用，减轻头文件互相包含
- 默认已生成 **OER + PER**；若只要 BER/XER，可加 `-no-gen-OER -no-gen-PER`

### 2.3 完整参数一览（asn1c 0.9.29，以 `asn1c -h` 为准）

#### 阶段 / 输出控制

| 参数 | 含义 |
|------|------|
| `-E` | 只跑解析，打印 ASN.1 语法树（不生成 C） |
| `-F` | 配合 `-E`：做完语义修复后再打印 |
| `-P` | 把生成的 C 打印到 stdout，不写文件 |
| `-R` | **只生成协议代码，不拷贝骨架**（与 `libasn1c` 分离时必加） |
| `-S <dir>` | 指定骨架目录（安装后默认 `install/share/asn1c`） |
| `-D <dir>` | 生成文件输出目录（默认当前目录；目录需已存在） |
| `-X` | 生成并打印 XML DTD |

#### 告警 / 调试

| 参数 | 含义 |
|------|------|
| `-Werror` | 警告当错误，有警告即失败 |
| `-Wdebug-lexer` | 词法阶段调试输出 |
| `-Wdebug-parser` | 语法阶段调试输出 |
| `-Wdebug-fixer` | 语义修复阶段调试输出 |
| `-Wdebug-compiler` | 代码生成阶段调试输出 |

#### 语言 / 代码风格（`-f*`）

| 参数 | 含义 |
|------|------|
| `-fbless-SIZE` | 允许给 INTEGER 等加 `SIZE()`（非标准，慎用） |
| `-fcompound-names` | 用更长的复合名，减少多模块标识符冲突 |
| `-findirect-choice` | CHOICE 成员生成为指针 |
| `-fincludes-quoted` | `#include` 用 `"..."` |
| `-fknown-extern-type=<name>` | 假装某类型已由外部提供，不生成其代码 |
| `-fline-refs` | 注释中带 ASN.1 行号 |
| `-fno-constraints` | 不生成约束检查代码（可减小体积） |
| `-fno-include-deps` | 不自动 `#include` 非关键依赖 |
| `-funnamed-unions` | 启用匿名 union |
| `-fwide-types` | 默认用 `INTEGER_t` 等宽类型，不用 `long`/`double` |

#### 编解码生成

| 参数 | 含义 |
|------|------|
| `-no-gen-OER` | **不**生成 OER（X.696）；默认会生成 |
| `-no-gen-PER` | **不**生成 PER/UPER（X.691）；默认会生成 |
| `-no-gen-example` | **不**生成示例转换程序 |
| `-gen-autotools` | 额外生成示例用的 `configure.ac` / `Makefile.am` |
| `-pdu=all` | PDU 表包含全部类型 |
| `-pdu=auto` | PDU 表只含「不被其它类型引用」的顶层类型 |
| `-pdu=TypeName` | 把指定类型加入 PDU 表（可写多次） |

> 说明：本版本 `asn1c -h` 用的是 `-no-gen-OER` / `-no-gen-PER`（默认开启编解码）。
> 旧文档里的 `-gen-OER` / `-gen-PER` 与当前二进制不一致，以 `-h` 为准。

#### 调试打印

| 参数 | 含义 |
|------|------|
| `-print-class-matrix` | 打印对象类矩阵（调试） |
| `-print-constraints` | 解释子类型约束（常配合 `-EF`） |
| `-print-lines` | 在 `-E` 输出中生成 `-- #line` 注释 |

随时查看本机已装版本：

```bash
asn1c -h
MANPATH="$(pwd)/install/share/man" man asn1c
```

### 2.4 最小可运行示例

写一个小规格 `hello.asn1`：

```asn1
HelloModule DEFINITIONS AUTOMATIC TAGS ::=
BEGIN
Hello ::= SEQUENCE {
    id    INTEGER (0..255),
    text  UTF8String
}
END
```

生成并编译（主机 gcc）：

```bash
mkdir -p /tmp/hello-asn && cd /tmp/hello-asn
cp /path/to/hello.asn1 .
asn1c -fcompound-names -no-gen-example hello.asn1
# 若需要自带示例编解码程序，去掉 -no-gen-example，会生成 converter-example.c
gcc -o hello-test -I. *.c
```

更完整的官方示例（装好 asn1c 后）：

```bash
cd examples/sample.source.PKIX1
make          # 生成 X.509 证书解码器
./enber ...   # 按该目录 README 使用
```

V2X 相关目录（需自行准备规格文件，见各目录 README）：

- `examples/sample.source.J2735` — SAE J2735 DSRC
- `examples/sample.source.1609.2` — IEEE 1609.2

---

## 3. 和业务工程怎么配合

推荐流程：

1. `./build.sh` 装好 `asn1c`。
2. 把协议 `.asn1` 放进业务仓库（或本工程独立目录）。
3. 用脚本调用 `install/bin/asn1c`，把生成的 `.c/.h` 输出到指定目录（如 `out1/`）。
4. 业务 CMake / Makefile 把生成的 `*.c` 编进目标库，并加上对应 `-I`。
5. 调用生成的 `uper_encode` / `uper_decode` / `ber_decode` 等 API 编解码。

示例（与 demo 一致：协议与 libasn1c 分开）：

```bash
cd demo && ./run_demo.sh
# 等价手动步骤：
mkdir -p asn_out libasn1c
asn1c -R -no-gen-example -findirect-choice -fincludes-quoted \
      -fline-refs -funnamed-unions -pdu=all -D asn_out asn_src/*.asn
# 骨架单独放到 libasn1c（不要混进 asn_out）
cp ../install/share/asn1c/*.[ch] libasn1c/
rm -f libasn1c/converter-example.*
```

编解码函数对照（摘自上游）：

| 编码族 | 编码 API | 解码 API |
|--------|----------|----------|
| BER/DER | `der_encode()` | `ber_decode()` |
| OER | `oer_encode()` | `oer_decode()` |
| UPER | `uper_encode()` | `uper_decode()` |
| XER | `xer_encode(...)` | `xer_decode()` |

详细手册：`doc/asn1c-usage.pdf`、`doc/asn1c-quick.pdf`，或：

```bash
MANPATH="$(pwd)/install/share/man" man asn1c
```

---

## 4. 常见问题

**Q: 为什么不装到 `/usr/local`？**  
A: 默认装到本目录 `install/`，和 libafl 的 `./install` 一样，方便多版本并存、不污染系统。需要合并到 libafl 安装树时：

```bash
./build.sh --prefix="$(cd ../.. && pwd)/install"
```

**Q: ARM 交叉编译要编 asn1c 吗？**  
A: 一般不用。asn1c 在 x86 上跑；只交叉编译「asn1c 生成出来的 C 代码」。

**Q: `make check` 失败？**  
A: 日常使用以 `asn1c -h` 和示例能跑为准。完整测试依赖环境较严（含 sanitizer），可用 `./build.sh` 跳过 `--check`。

**Q: 本树对上游运行库改了什么？**  
A: 修了 `skeletons/constr_SEQUENCE.c`、`constr_SET.c` 的约束检查：0.9.29 原版在成员无自带约束时直接 `return`，只查到第一个这样的成员就结束，嵌套字段越界（如 `rtsId=300` 超出 0..255）`asn_check_constraints` / `check()` 查不出，编码时才失败。现在逐个成员检查、失败才返回。旧工程（如 RSU `libasn1c`）若仍是原版运行库，存在同样漏检。

**Q: bison 版本？**  
A: 上游标注偏好 bison 2.x；本树若已带预生成解析器，bison 3.x 通常仍可直接 `./build.sh`。若 `autoreconf` 报错，再考虑安装 bison 2 或使用已带 `configure` 的源码包。

---

## 5. 版本与许可

- 源码版本：见 `configure.ac` / `ChangeLog`（当前树为 asn1c 0.9.29 系）
- 许可：见 [LICENSE](LICENSE)
