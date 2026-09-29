#!/bin/bash
# ============================================================================
#  libafl 构建脚本
#
#  用法:
#    ./build.sh              # 本机构建 + 运行单元测试 + 安装到 ./install
#    ./build.sh --arm        # arm-linux-gnueabi 交叉编译（不跑测试，装到 ./install-arm）
#    ./build.sh --doc        # 生成 Doxygen 文档
#    ./build.sh --coverage   # 一键生成单元测试覆盖率报告（行 + 分支）
#    ./build.sh --demo       # 构建、安装并运行 examples/demo_app（用 install 里的二进制）
#    ./build.sh --shared     # 构建动态库
#    ./build.sh --no-install # 只编译不安装
#    ./build.sh --prefix=DIR # 指定安装目录（默认本机 install/，交叉 install-arm/）
#    ./build.sh --clean      # 清理构建与安装目录
# ============================================================================
set -e
cd "$(dirname "$0")"

BUILD_DIR=build
INSTALL_DIR=""
DO_INSTALL=1
RUN_DEMO=0
EXTRA_ARGS=""

for arg in "$@"; do
    case "$arg" in
        --arm)
            echo "==> cross build for arm-linux-gnueabi"
            BUILD_DIR=build-arm
            INSTALL_DIR=install-arm
            EXTRA_ARGS="$EXTRA_ARGS -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-linux-gnueabi.cmake -DAFL_BUILD_TESTS=OFF"
            # 交叉环境无 openssl 开发库，默认关闭 SSL
            EXTRA_ARGS="$EXTRA_ARGS -DAFL_ENABLE_SSL=OFF"
            ;;
        --doc)
            echo "==> generate Doxygen docs"
            if ! command -v doxygen >/dev/null 2>&1; then
                echo "ERROR: doxygen not found. Install with: sudo apt-get install -y doxygen" >&2
                exit 1
            fi
            doxygen Doxyfile
            echo "docs written to docs/html"
            exit 0
            ;;
        --coverage)
            echo "==> generate unit-test coverage report (line + branch)"
            exec ./scripts/gen_coverage_report.sh
            ;;
        --demo)
            RUN_DEMO=1
            EXTRA_ARGS="$EXTRA_ARGS -DAFL_BUILD_EXAMPLES=ON"
            ;;
        --shared) EXTRA_ARGS="$EXTRA_ARGS -DAFL_BUILD_SHARED=ON" ;;
        --no-install)
            DO_INSTALL=0
            ;;
        --prefix=*)
            INSTALL_DIR="${arg#--prefix=}"
            ;;
        --clean)
            rm -rf build build-arm build-coverage coverage-report install install-arm
            echo "cleaned"
            exit 0
            ;;
        *) EXTRA_ARGS="$EXTRA_ARGS $arg" ;;
    esac
done

# 默认安装到工程内独立目录，方便本地测试（不碰 /usr/local）
if [ -z "$INSTALL_DIR" ]; then
    if [ "$BUILD_DIR" = "build-arm" ]; then
        INSTALL_DIR=install-arm
    else
        INSTALL_DIR=install
    fi
fi
# 相对路径转成绝对路径，避免 cmake --install 歧义
case "$INSTALL_DIR" in
    /*) ;;
    *) INSTALL_DIR="$(pwd)/$INSTALL_DIR" ;;
esac

echo "==> configure ($BUILD_DIR)"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    $EXTRA_ARGS .

# clangd / IDE 跳转：工程根目录需要 compile_commands.json
if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    ln -sfn "$BUILD_DIR/compile_commands.json" compile_commands.json
fi

echo "==> build"
cmake --build "$BUILD_DIR" -j"$(nproc)"

if [ "$BUILD_DIR" = "build" ]; then
    echo "==> run tests"
    (cd "$BUILD_DIR" && ctest --output-on-failure)
fi

if [ "$DO_INSTALL" = "1" ]; then
    echo "==> install -> $INSTALL_DIR"
    cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"
    echo "    bin:     $INSTALL_DIR/bin"
    echo "    lib:     $INSTALL_DIR/lib"
    echo "    include: $INSTALL_DIR/include"
fi

if [ "$RUN_DEMO" = "1" ]; then
    if [ "$DO_INSTALL" = "1" ] && [ -x "$INSTALL_DIR/bin/afl_demo_app" ]; then
        DEMO_BIN="$INSTALL_DIR/bin/afl_demo_app"
    else
        DEMO_BIN="$BUILD_DIR/bin/afl_demo_app"
    fi
    if [ ! -x "$DEMO_BIN" ]; then
        echo "ERROR: demo binary not found: $DEMO_BIN" >&2
        exit 1
    fi
    echo "==> run demo: $DEMO_BIN"
    "$DEMO_BIN"
fi

echo "==> done"
