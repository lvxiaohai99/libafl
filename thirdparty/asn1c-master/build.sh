#!/bin/bash
# ============================================================================
#  asn1c 构建脚本（本仓库 thirdparty 封装）
#
#  用法:
#    ./build.sh                 # 本机构建并安装到 ./install
#    ./build.sh --prefix=DIR    # 指定安装目录（绝对或相对路径）
#    ./build.sh --check         # 构建后运行 make check（较慢）
#    ./build.sh --no-install    # 只编译不安装
#    ./build.sh --clean         # 清理 build/ 与 install/
#    ./build.sh --jobs=N        # 并行编译任务数（默认 nproc）
#
#  说明:
#    - asn1c 是「宿主机」代码生成工具，一般不需要交叉编译；
#      生成的 .c/.h 再交给目标机（含 ARM）工具链去编。
#    - 默认不写系统目录，产物落在本目录 install/，方便本地测试。
# ============================================================================
set -euo pipefail
cd "$(dirname "$0")"

ROOT="$(pwd)"
BUILD_DIR="${ROOT}/build"
INSTALL_DIR="${ROOT}/install"
DO_INSTALL=1
DO_CHECK=0
JOBS="$(nproc 2>/dev/null || echo 4)"

for arg in "$@"; do
    case "$arg" in
        --prefix=*)
            INSTALL_DIR="${arg#--prefix=}"
            ;;
        --check)
            DO_CHECK=1
            ;;
        --no-install)
            DO_INSTALL=0
            ;;
        --jobs=*)
            JOBS="${arg#--jobs=}"
            ;;
        --clean)
            rm -rf "${BUILD_DIR}" "${ROOT}/install"
            # 若源码树里曾做过 in-tree configure，一并清掉，避免污染
            if [ -f "${ROOT}/Makefile" ]; then
                make -C "${ROOT}" distclean >/dev/null 2>&1 || true
            fi
            echo "cleaned"
            exit 0
            ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            echo "ERROR: unknown option: $arg (try --help)" >&2
            exit 1
            ;;
    esac
done

# 相对路径转绝对路径，避免 make install 歧义
case "$INSTALL_DIR" in
    /*) ;;
    *) INSTALL_DIR="${ROOT}/${INSTALL_DIR}" ;;
esac

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "ERROR: missing command: $1" >&2
        echo "  Ubuntu/Debian: sudo apt-get install -y build-essential autoconf automake libtool bison flex" >&2
        exit 1
    fi
}

need_cmd cc
need_cmd make
need_cmd autoconf
need_cmd automake
need_cmd libtoolize
need_cmd bison
need_cmd flex

# 源码树里若残留 in-tree Makefile / 硬编码旧路径，先尽量清掉
if [ -f "${ROOT}/Makefile" ]; then
    echo "==> distclean in-tree leftovers"
    make -C "${ROOT}" distclean >/dev/null 2>&1 || true
    # distclean 失败时（例如 missing 脚本路径写死），手动清关键残留
    rm -f "${ROOT}/Makefile" "${ROOT}/config.status" "${ROOT}/config.log" \
          "${ROOT}/config.h" "${ROOT}/stamp-h1" "${ROOT}/libtool"
fi

# 用本机 automake/autoconf 重新生成（避免源码包里写死 aclocal-1.15）
echo "==> autoreconf -fi"
(
    cd "${ROOT}"
    autoreconf -fi
)

# autoreconf 后 configure 可能已变，始终在干净 build 目录重新配置
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
echo "==> configure --prefix=${INSTALL_DIR}"
( cd "${BUILD_DIR}" && "${ROOT}/configure" --prefix="${INSTALL_DIR}" )

echo "==> build (-j${JOBS})"
make -C "${BUILD_DIR}" -j"${JOBS}"

if [ "$DO_CHECK" -eq 1 ]; then
    echo "==> make check"
    make -C "${BUILD_DIR}" check
fi

if [ "$DO_INSTALL" -eq 1 ]; then
    echo "==> install -> ${INSTALL_DIR}"
    make -C "${BUILD_DIR}" install
    echo
    echo "done. layout:"
    echo "  ${INSTALL_DIR}/bin/asn1c"
    echo "  ${INSTALL_DIR}/bin/unber   # BER 解码辅助"
    echo "  ${INSTALL_DIR}/bin/enber   # BER 编码辅助"
    echo "  ${INSTALL_DIR}/share/asn1c # 运行时骨架代码 (skeletons)"
    echo
    echo "quick test:"
    echo "  export PATH=${INSTALL_DIR}/bin:\$PATH"
    echo "  asn1c -h"
fi
