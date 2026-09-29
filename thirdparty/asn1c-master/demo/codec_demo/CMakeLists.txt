# asn1 codec demo：依赖 demo/asn_out + demo/libasn1c + afl/asn1/Asn1Cpp.h
# 纳入 libafl 主工程 compile_commands，供 clangd / IDE 跳转。

cmake_minimum_required(VERSION 3.10)

get_filename_component(AFL_ASN1_DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(AFL_ASN_OUT   "${AFL_ASN1_DEMO_DIR}/asn_out")
set(AFL_LIBASN1C  "${AFL_ASN1_DEMO_DIR}/libasn1c")
# 作为 libafl 子工程时用 CMAKE_SOURCE_DIR；单独配置时向上找到 libafl 根
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    get_filename_component(AFL_SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
else()
    set(AFL_SRC_ROOT "${CMAKE_SOURCE_DIR}")
endif()

if(NOT EXISTS "${AFL_ASN_OUT}/MessageFrame.h" OR NOT EXISTS "${AFL_LIBASN1C}/asn_application.h")
    message(STATUS "asn1_codec_demo: skip (run thirdparty/asn1c-master/demo/run_demo.sh first)")
    return()
endif()

file(GLOB AFL_ASN_OUT_SOURCES CONFIGURE_DEPENDS "${AFL_ASN_OUT}/*.c")
file(GLOB AFL_LIBASN1C_SOURCES CONFIGURE_DEPENDS "${AFL_LIBASN1C}/*.c")

add_library(afl_asn1_codec_objs STATIC
    ${AFL_ASN_OUT_SOURCES}
    ${AFL_LIBASN1C_SOURCES}
)
set_target_properties(afl_asn1_codec_objs PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME afl_asn1_codec
)
target_include_directories(afl_asn1_codec_objs PUBLIC
    "${AFL_ASN_OUT}"
    "${AFL_LIBASN1C}"
)
# asn1c 生成代码告警较多，demo 库不强行 -Wall
target_compile_options(afl_asn1_codec_objs PRIVATE -Wno-unused-parameter -Wno-unused-function)

add_executable(afl_asn1_codec_demo
    main.cpp
)
target_include_directories(afl_asn1_codec_demo PRIVATE
    "${AFL_SRC_ROOT}"          # → #include "afl/asn1/Asn1Cpp.h"
    "${AFL_SRC_ROOT}/thirdparty"   # → #include "nlohmann/json.hpp"（Asn1Json 输入解析）
    "${AFL_ASN_OUT}"
    "${AFL_LIBASN1C}"
)
target_link_libraries(afl_asn1_codec_demo PRIVATE afl_asn1_codec_objs)
target_compile_features(afl_asn1_codec_demo PRIVATE cxx_std_11)
target_compile_options(afl_asn1_codec_demo PRIVATE -Wall)

# 与 Asn1Cpp.h / Asn1Json.h 同目录的空壳 TU，写入 compile_commands，供单独打开头文件时跳转
add_library(afl_asn1_ide OBJECT "${AFL_SRC_ROOT}/afl/asn1/Asn1Ide.cpp")
target_include_directories(afl_asn1_ide PRIVATE
    "${AFL_SRC_ROOT}"
    "${AFL_SRC_ROOT}/thirdparty"
    "${AFL_ASN_OUT}"
    "${AFL_LIBASN1C}"
)
target_compile_features(afl_asn1_ide PRIVATE cxx_std_11)

set_target_properties(afl_asn1_codec_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 不强制安装；需要时可跟随 examples
install(TARGETS afl_asn1_codec_demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
