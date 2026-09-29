#!/bin/bash
# ============================================================================
#  编译并运行 asn1 codec demo
#  优先走 libafl CMake（写入 compile_commands，支持 IDE 跳转）
#
#  用法：
#    ./run.sh                  # 生成协议（若需要）→ cmake 编译 → roundtrip
#    ./run.sh decode --uper samples/rsi.uper
#    ./run.sh --clean
# ============================================================================
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CODEC_DIR="$(cd "$(dirname "$0")" && pwd)"
ASN1C_ROOT="$(cd "${DEMO_DIR}/.." && pwd)"
AFL_ROOT="$(cd "${ASN1C_ROOT}/../.." && pwd)"
AFL_BUILD="${AFL_ROOT}/build"
BIN="${AFL_BUILD}/bin/afl_asn1_codec_demo"

if [ "${1:-}" = "--clean" ]; then
    rm -rf "${CODEC_DIR}/samples" "${CODEC_DIR}/build"
    # 不删整个 libafl/build，只提示
    echo "cleaned codec samples; rebuild via: cd ${AFL_ROOT} && ./build.sh"
    exit 0
fi

if [ ! -f "${DEMO_DIR}/asn_out/MessageFrame.h" ] || [ ! -f "${DEMO_DIR}/libasn1c/asn_application.h" ]; then
    echo "==> generate asn_out + libasn1c"
    "${DEMO_DIR}/run_demo.sh"
fi

echo "==> configure/build via libafl CMake (updates compile_commands.json)"
cmake -S "${AFL_ROOT}" -B "${AFL_BUILD}" -DCMAKE_BUILD_TYPE=Release -DAFL_BUILD_EXAMPLES=ON
cmake --build "${AFL_BUILD}" --target afl_asn1_codec_demo -j"$(nproc 2>/dev/null || echo 4)"

# 保证 IDE 能找到编译数据库
if [ -f "${AFL_BUILD}/compile_commands.json" ]; then
    ln -sfn "${AFL_BUILD}/compile_commands.json" "${AFL_ROOT}/compile_commands.json"
fi

if [ ! -x "${BIN}" ]; then
    echo "ERROR: missing ${BIN}" >&2
    exit 1
fi

echo "==> run"
cd "${CODEC_DIR}"
"${BIN}" "$@"
