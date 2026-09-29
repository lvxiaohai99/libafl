#!/bin/bash
# ============================================================================
#  demo：把协议 ASN 与 libasn1c（骨架 runtime）分开生成
#
#    asn_src/  →  ASN.1 规格（.asn）
#    asn_out/  →  仅协议生成的 .c/.h（asn1c -R，不含骨架）
#    libasn1c/   →  asn1c 骨架/runtime（从 install/share/asn1c 同步）
#
#  用法：
#    ./run_demo.sh                 # 生成 asn_out + 同步 libasn1c + 抽检
#    ./run_demo.sh --clean         # 清空 asn_out、libasn1c
#    ./run_demo.sh --rebuild-asn1c # 先跑上层 build.sh 再生成
#
#  参数与既有 V2X 工程对齐（多了 -R 以不拷贝骨架）：
#    -R -no-gen-example -findirect-choice -fincludes-quoted
#    -fline-refs -funnamed-unions -pdu=all -D asn_out
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ "$(basename "$SCRIPT_DIR")" = "demo" ]; then
    DEMO_DIR="$SCRIPT_DIR"
    ASN1C_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
else
    DEMO_DIR="$SCRIPT_DIR/demo"
    ASN1C_ROOT="$SCRIPT_DIR"
fi

ASN_SRC="${DEMO_DIR}/asn_src"
ASN_OUT="${DEMO_DIR}/asn_out"
LIBASN1C="${DEMO_DIR}/libasn1c"
ASN1C_BIN="${ASN1C_ROOT}/install/bin/asn1c"
SKELETON_DIR="${ASN1C_ROOT}/install/share/asn1c"
REBUILD=0

for arg in "$@"; do
    case "$arg" in
        --clean)
            rm -rf "${ASN_OUT}" "${LIBASN1C}"
            mkdir -p "${ASN_OUT}" "${LIBASN1C}"
            echo "cleaned ${ASN_OUT} and ${LIBASN1C}"
            exit 0
            ;;
        --rebuild-asn1c)
            REBUILD=1
            ;;
        -h|--help)
            sed -n '2,18p' "$0"
            exit 0
            ;;
        *)
            echo "ERROR: unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

if [ ! -x "$ASN1C_BIN" ] || [ "$REBUILD" -eq 1 ]; then
    echo "==> asn1c not ready, running ${ASN1C_ROOT}/build.sh"
    if [ ! -x "${ASN1C_ROOT}/build.sh" ]; then
        echo "ERROR: missing ${ASN1C_ROOT}/build.sh" >&2
        exit 1
    fi
    "${ASN1C_ROOT}/build.sh"
fi

if [ ! -x "$ASN1C_BIN" ]; then
    echo "ERROR: asn1c binary missing: $ASN1C_BIN" >&2
    exit 1
fi
if [ ! -d "$SKELETON_DIR" ]; then
    echo "ERROR: skeleton dir missing: $SKELETON_DIR (re-run build.sh)" >&2
    exit 1
fi

mapfile -t ASN_FILES < <(find "$ASN_SRC" -maxdepth 1 -type f -name '*.asn' | sort)
if [ "${#ASN_FILES[@]}" -eq 0 ]; then
    echo "ERROR: no .asn under ${ASN_SRC}" >&2
    exit 1
fi

echo "==> asn sources: ${#ASN_FILES[@]} files in ${ASN_SRC}"

# ---------- 1) 协议代码 → asn_out（-R：不拷贝骨架）----------
rm -rf "${ASN_OUT}"
mkdir -p "${ASN_OUT}"

echo "==> generate protocol code -> ${ASN_OUT}  (-R = no skeletons)"
"$ASN1C_BIN" \
    -R \
    -no-gen-example \
    -findirect-choice \
    -fincludes-quoted \
    -fline-refs \
    -funnamed-unions \
    -pdu=all \
    -D "${ASN_OUT}" \
    "${ASN_FILES[@]}"

# ---------- 2) 骨架 runtime → libasn1c ----------
echo "==> sync skeletons -> ${LIBASN1C}"
mkdir -p "${LIBASN1C}"
# 清掉旧的 .c/.h，保留 README 等说明文件
find "${LIBASN1C}" -maxdepth 1 -type f \( -name '*.c' -o -name '*.h' \) -delete
# 只同步源码与头文件；排除示例
find "$SKELETON_DIR" -maxdepth 1 -type f \( -name '*.c' -o -name '*.h' \) \
    ! -name 'converter-example.c' \
    ! -name 'converter-example.h' \
    -exec cp -a {} "${LIBASN1C}/" \;
# 若说明文件被误删，补一份最短提示
if [ ! -f "${LIBASN1C}/README.md" ]; then
    cat > "${LIBASN1C}/README.md" <<'EOF'
# libasn1c — asn1c 骨架 / runtime

由 `../run_demo.sh` 从 `../../install/share/asn1c/` 同步。
对应 RSU 的 `libasn1c/`。业务编译：`-Iasn_out -Ilibasn1c`。
EOF
fi

# ---------- 3) 确认两边无交集 ----------
mapfile -t OUT_NAMES < <(find "$ASN_OUT" -maxdepth 1 -type f \( -name '*.c' -o -name '*.h' \) -printf '%f\n' | sort)
mapfile -t LIB_NAMES < <(find "$LIBASN1C" -maxdepth 1 -type f \( -name '*.c' -o -name '*.h' \) -printf '%f\n' | sort)
OVERLAP="$(comm -12 <(printf '%s\n' "${OUT_NAMES[@]}") <(printf '%s\n' "${LIB_NAMES[@]}") || true)"
if [ -n "${OVERLAP}" ]; then
    echo "ERROR: asn_out and libasn1c still share files:" >&2
    echo "${OVERLAP}" >&2
    exit 1
fi

H_COUNT="$(find "$ASN_OUT" -maxdepth 1 -type f -name '*.h' | wc -l)"
C_COUNT="$(find "$ASN_OUT" -maxdepth 1 -type f -name '*.c' | wc -l)"
LIB_H="$(find "$LIBASN1C" -maxdepth 1 -type f -name '*.h' | wc -l)"
LIB_C="$(find "$LIBASN1C" -maxdepth 1 -type f -name '*.c' | wc -l)"

echo
echo "==> result (separated)"
echo "  asn_out/  protocol: ${C_COUNT} .c + ${H_COUNT} .h"
echo "  libasn1c/   runtime : ${LIB_C} .c + ${LIB_H} .h"
echo "  overlap           : 0 (ok)"

NEED_HEADERS=(
    MessageFrame.h
    BasicSafetyMessage.h
    MapData.h
    SPAT.h
    RoadSideInformation.h
    RoadsideSafetyMessage.h
)
MISSING=0
for h in "${NEED_HEADERS[@]}"; do
    if [ ! -f "${ASN_OUT}/${h}" ]; then
        echo "  MISSING protocol: ${h}"
        MISSING=1
    else
        echo "  ok protocol: ${h}"
    fi
done
for h in asn_application.h ber_decoder.h INTEGER.h constr_CHOICE.h; do
    if [ ! -f "${LIBASN1C}/${h}" ]; then
        echo "  MISSING libasn1c: ${h}"
        MISSING=1
    else
        echo "  ok libasn1c: ${h}"
    fi
done

if [ "$H_COUNT" -lt 1 ] || [ "$C_COUNT" -lt 1 ]; then
    echo "ERROR: asn1c produced no protocol .c/.h" >&2
    exit 1
fi
if [ "$MISSING" -ne 0 ]; then
    echo "ERROR: expected files missing" >&2
    exit 1
fi

# 语法抽检：协议代码依赖 libasn1c 头，必须两个 -I
if command -v gcc >/dev/null 2>&1; then
    echo "==> syntax smoke (gcc -I asn_out -I libasn1c)"
    gcc -fsyntax-only -I"${ASN_OUT}" -I"${LIBASN1C}" "${ASN_OUT}/MessageFrame.c" \
        || { echo "ERROR: gcc syntax check failed" >&2; exit 1; }
    echo "  syntax ok"
fi

echo
echo "PASS: protocol in asn_out/, runtime in libasn1c/ (same layout as RSU MessageFrame/src + libasn1c)"
echo "  compile tip: gcc -Iasn_out -Ilibasn1c  …  and link both sets of .c (or build libasn1c as shared lib)"
