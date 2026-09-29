# ============================================================================
#  libafl —— 通用 C++11 工具库（Linux）v1.1.0
#
#  目标:
#    afl / afl::afl : 工具库本体（默认静态库，AFL_BUILD_SHARED=ON 切换动态库）
#    afl_tests      : 单元测试（AFL_BUILD_TESTS=ON）
#
#  选项:
#    AFL_BUILD_SHARED  构建动态库 (默认 OFF)
#    AFL_ENABLE_SSL    启用 SSL 模块 (默认 ON，找不到 OpenSSL 自动关闭)
#    AFL_BUILD_TESTS   构建单元测试 (默认 ON)
# ============================================================================
cmake_minimum_required(VERSION 3.10)
project(libafl VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# 供 clangd / IDE「转到定义」：生成 build/compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(AFL_BUILD_SHARED   "Build afl as a shared library" OFF)
option(AFL_ENABLE_SSL     "Enable SSL module (SslHelper)" ON)
option(AFL_BUILD_TESTS    "Build unit tests" ON)
option(AFL_BUILD_EXAMPLES "Build example apps (demo_app)" ON)
option(AFL_ENABLE_COVERAGE "Enable gcov line/branch coverage (Debug-like flags)" OFF)

# 覆盖率构建：-O0 -g 保证统计准确；加 -DNDEBUG 关闭 assert（与 Release 行为一致，
# 避免 EventLoop 等「先在主线程构造、再在子线程 loop」的测试在 Debug 下误触发断言）
if(AFL_ENABLE_COVERAGE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage -O0 -g -DNDEBUG)
    add_link_options(--coverage)
    message(STATUS "AFL_ENABLE_COVERAGE=ON: Debug + --coverage + -DNDEBUG")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ---------------------------------------------------------------------------
# 源文件（在 add_library 前定稿，含 SSL 过滤）
# ---------------------------------------------------------------------------
file(GLOB_RECURSE AFL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/afl/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/afl/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/afl/*.c)

set(AFL_HAVE_SSL OFF)
if(AFL_ENABLE_SSL)
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        set(AFL_HAVE_SSL ON)
    else()
        message(WARNING "OpenSSL not found, SslHelper is disabled")
    endif()
endif()

if(NOT AFL_HAVE_SSL)
    list(FILTER AFL_SOURCES EXCLUDE REGEX "SslHelper\\.(cpp|c)$")
endif()
# Asn1Ide.cpp 只给 clangd 用（由 codec_demo 的 afl_asn1_ide 目标编译），不要编进 libafl
list(FILTER AFL_SOURCES EXCLUDE REGEX "Asn1Ide\\.cpp$")

if(AFL_BUILD_SHARED)
    add_library(afl SHARED ${AFL_SOURCES})
else()
    add_library(afl STATIC ${AFL_SOURCES})
endif()

add_library(afl::afl ALIAS afl)

set_target_properties(afl PROPERTIES
    OUTPUT_NAME afl
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    EXPORT_NAME afl
)

target_include_directories(afl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/spdlog-src>
)

target_compile_options(afl PRIVATE -Wall -Wno-unused-function -Wno-unused-parameter)

find_package(Threads REQUIRED)
target_link_libraries(afl PUBLIC Threads::Threads)

if(AFL_HAVE_SSL)
    target_link_libraries(afl PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(afl PUBLIC AFL_ENABLE_SSL=1)
endif()

# ---------------------------------------------------------------------------
# 单元测试
# ---------------------------------------------------------------------------
if(AFL_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
    enable_testing()
    add_subdirectory(test)
endif()

if(AFL_BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo_app/CMakeLists.txt)
    add_subdirectory(examples/demo_app)
endif()

# asn1 codec demo：写入同一份 compile_commands，支持 IDE 跳到 afl/asn1 与 asn_out
if(AFL_BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/asn1c-master/demo/codec_demo/CMakeLists.txt)
    add_subdirectory(thirdparty/asn1c-master/demo/codec_demo)
endif()

# ---------------------------------------------------------------------------
# 安装 + CMake 包
# ---------------------------------------------------------------------------
install(TARGETS afl
        EXPORT aflTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/afl
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# 随包装 spdlog 头，保证开箱即用
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog/include/spdlog
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

install(EXPORT aflTargets
        FILE aflTargets.cmake
        NAMESPACE afl::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/afl)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/aflConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/aflConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/afl)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/aflConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/aflConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/aflConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/afl)
