#!/usr/bin/env bash
# ============================================================================
#  一键生成 C/C++ 单元测试覆盖率报告（行覆盖 + 分支覆盖）
#
#  依赖: cmake, g++, gcov, lcov, genhtml
#    Ubuntu/Debian: sudo apt-get install -y lcov
#
#  用法（本仓库）:
#    ./scripts/gen_coverage_report.sh
#    ./build.sh --coverage          # 等价入口
#
#  用法（其它 CMake 项目可借鉴）:
#    COVERAGE_SRC_ROOT=src \
#    COVERAGE_CMAKE_ARGS="-DMYLIB_ENABLE_COVERAGE=ON" \
#    COVERAGE_TEST_TARGET=my_tests \
#    ./scripts/gen_coverage_report.sh
#
#  环境变量（均可选）:
#    BUILD_DIR            构建目录，默认 build-coverage
#    REPORT_DIR           报告输出目录，默认 coverage-report
#    COVERAGE_SRC_ROOT    统计的源码相对根（多个用空格），默认 afl
#    COVERAGE_EXCLUDE     lcov --remove 额外模式（空格分隔）
#    COVERAGE_CMAKE_ARGS  传给 cmake 的额外参数
#    COVERAGE_TEST_CMD    自定义跑测试命令；空则用 ctest
#    MIN_LINE_COVERAGE    行覆盖率下限（0-100），未达则脚本失败；默认不检查
#    MIN_BRANCH_COVERAGE  分支覆盖率下限；默认不检查
#    OPEN_BROWSER         设为 1 时尝试打开 HTML 报告
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# 若脚本在 <repo>/scripts/ 下，项目根为上一级；否则为脚本所在目录
if [[ -f "${SCRIPT_DIR}/../CMakeLists.txt" ]]; then
    PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
else
    PROJECT_ROOT="${SCRIPT_DIR}"
fi
cd "${PROJECT_ROOT}"

BUILD_DIR="${BUILD_DIR:-build-coverage}"
REPORT_DIR="${REPORT_DIR:-coverage-report}"
COVERAGE_SRC_ROOT="${COVERAGE_SRC_ROOT:-afl}"
COVERAGE_CMAKE_ARGS="${COVERAGE_CMAKE_ARGS:--DAFL_ENABLE_COVERAGE=ON -DAFL_BUILD_TESTS=ON}"
COVERAGE_EXCLUDE="${COVERAGE_EXCLUDE:-}"
MIN_LINE_COVERAGE="${MIN_LINE_COVERAGE:-}"
MIN_BRANCH_COVERAGE="${MIN_BRANCH_COVERAGE:-}"
OPEN_BROWSER="${OPEN_BROWSER:-0}"

die() { echo "ERROR: $*" >&2; exit 1; }

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "未找到命令 '$1'。请安装后重试。"
}

need_cmd cmake
need_cmd gcov
need_cmd lcov
need_cmd genhtml

echo "==> [1/5] 清理旧覆盖率数据与报告目录"
rm -rf "${BUILD_DIR}" "${REPORT_DIR}"
mkdir -p "${REPORT_DIR}"

echo "==> [2/5] 配置并编译（覆盖率插桩）"
# shellcheck disable=SC2086
cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug ${COVERAGE_CMAKE_ARGS} .
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "==> [3/5] 运行单元测试以生成 .gcda"
if [[ -n "${COVERAGE_TEST_CMD:-}" ]]; then
    # shellcheck disable=SC2086
    (cd "${BUILD_DIR}" && eval ${COVERAGE_TEST_CMD})
else
    (cd "${BUILD_DIR}" && ctest --output-on-failure)
fi

INFO_RAW="${REPORT_DIR}/coverage_raw.info"
INFO_FILTERED="${REPORT_DIR}/coverage.info"
SUMMARY_TXT="${REPORT_DIR}/summary.txt"
HTML_DIR="${REPORT_DIR}/html"

# lcov 1.x / 2.x 兼容参数
LCOV_IGNORE=()
GENHTML_IGNORE=()
if lcov --help 2>&1 | grep -q -- '--ignore-errors'; then
    # 常见噪声：源码行号 mismatch、未使用的 gcda 等（不同 lcov 版本支持集合不同）
    LCOV_IGNORE=(--ignore-errors mismatch,unused,empty,corrupt,source --keep-going)
fi
if genhtml --help 2>&1 | grep -q -- '--ignore-errors'; then
    GENHTML_IGNORE=(--ignore-errors source,unused --keep-going)
fi

echo "==> [4/5] 采集覆盖率（含分支）"
# shellcheck disable=SC2086
lcov --capture \
    --directory "${BUILD_DIR}" \
    --base-directory "${PROJECT_ROOT}" \
    --output-file "${INFO_RAW}" \
    --rc branch_coverage=1 \
    --rc geninfo_auto_base=1 \
    "${LCOV_IGNORE[@]}" \
    >/dev/null

# 默认剔除系统头、第三方、测试与构建产物
REMOVE_PATTERNS=(
    '/usr/*'
    '*/thirdparty/*'
    '*/googletest/*'
    '*/test/*'
    '*/CMakeFiles/*'
    '*/build*/*'
)
# shellcheck disable=SC2206
EXTRA_REMOVE=(${COVERAGE_EXCLUDE})
REMOVE_PATTERNS+=("${EXTRA_REMOVE[@]}")

# 若指定了源码根，用 --extract 收紧到业务代码
EXTRACT_ARGS=()
for root in ${COVERAGE_SRC_ROOT}; do
    EXTRACT_ARGS+=("${PROJECT_ROOT}/${root}/*")
done

if ((${#EXTRACT_ARGS[@]} > 0)); then
    lcov --extract "${INFO_RAW}" "${EXTRACT_ARGS[@]}" \
        --output-file "${INFO_FILTERED}.tmp" \
        --rc branch_coverage=1 \
        "${LCOV_IGNORE[@]}" \
        >/dev/null || cp "${INFO_RAW}" "${INFO_FILTERED}.tmp"
else
    cp "${INFO_RAW}" "${INFO_FILTERED}.tmp"
fi

lcov --remove "${INFO_FILTERED}.tmp" "${REMOVE_PATTERNS[@]}" \
    --output-file "${INFO_FILTERED}" \
    --rc branch_coverage=1 \
    "${LCOV_IGNORE[@]}" \
    >/dev/null || mv "${INFO_FILTERED}.tmp" "${INFO_FILTERED}"
rm -f "${INFO_FILTERED}.tmp"

echo "==> [5/5] 生成 HTML / 文本摘要"
genhtml "${INFO_FILTERED}" \
    --output-directory "${HTML_DIR}" \
    --title "Unit Test Coverage Report" \
    --legend \
    --show-details \
    --branch-coverage \
    --rc branch_coverage=1 \
    "${GENHTML_IGNORE[@]}" \
    >/dev/null || genhtml "${INFO_FILTERED}" \
        --output-directory "${HTML_DIR}" \
        --title "Unit Test Coverage Report" \
        --legend \
        --show-details \
        --branch-coverage \
        --rc branch_coverage=1 \
        >/dev/null


# 文本摘要（含行/分支百分比）
{
    echo "=============================================="
    echo "  Unit Test Coverage Summary"
    echo "  Project : ${PROJECT_ROOT}"
    echo "  Time    : $(date '+%Y-%m-%d %H:%M:%S')"
    echo "=============================================="
    echo
    lcov --summary "${INFO_FILTERED}" --rc branch_coverage=1 "${LCOV_IGNORE[@]}" 2>&1 || true
    echo
    echo "HTML report: ${PROJECT_ROOT}/${HTML_DIR}/index.html"
    echo "lcov data  : ${PROJECT_ROOT}/${INFO_FILTERED}"
} | tee "${SUMMARY_TXT}"

# 可选门槛检查
parse_pct() {
    # 从 summary 中提取 "lines......: 42.0%" / "branches...: 10.0%"
    local kind="$1"
    local line
    line="$(lcov --summary "${INFO_FILTERED}" --rc branch_coverage=1 "${LCOV_IGNORE[@]}" 2>&1 \
        | grep -iE "^[[:space:]]*${kind}" | head -1 || true)"
    echo "${line}" | sed -n 's/.*:[[:space:]]*\([0-9.]*\)%.*/\1/p'
}

if [[ -n "${MIN_LINE_COVERAGE}" ]]; then
    line_pct="$(parse_pct lines)"
    [[ -n "${line_pct}" ]] || die "无法解析行覆盖率"
    awk -v a="${line_pct}" -v b="${MIN_LINE_COVERAGE}" 'BEGIN{exit !(a+0 >= b+0)}' \
        || die "行覆盖率 ${line_pct}% < 门槛 ${MIN_LINE_COVERAGE}%"
fi
if [[ -n "${MIN_BRANCH_COVERAGE}" ]]; then
    br_pct="$(parse_pct branches)"
    [[ -n "${br_pct}" ]] || die "无法解析分支覆盖率"
    awk -v a="${br_pct}" -v b="${MIN_BRANCH_COVERAGE}" 'BEGIN{exit !(a+0 >= b+0)}' \
        || die "分支覆盖率 ${br_pct}% < 门槛 ${MIN_BRANCH_COVERAGE}%"
fi

if [[ "${OPEN_BROWSER}" == "1" ]]; then
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "${HTML_DIR}/index.html" >/dev/null 2>&1 || true
    fi
fi

echo "==> done"
