# ============================================================================
#  libafl 单元测试（googletest 1.12.1，源码内置）
# ============================================================================
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/googletest
                 ${CMAKE_CURRENT_BINARY_DIR}/gtest-build EXCLUDE_FROM_ALL)

add_executable(afl_tests
    main.cpp
    BaseTest.cpp
    StringTest.cpp
    CryptoTest.cpp
    TimeTest.cpp
    FileConfigTest.cpp
    ConcurrencyTest.cpp
    LogTest.cpp
    NetTest.cpp
    FrameworkTest.cpp
    ProcessTest.cpp
    ExtraModulesTest.cpp
    SignalTest.cpp
)

target_include_directories(afl_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/googletest/googletest/include
)

target_link_libraries(afl_tests PRIVATE afl gtest Threads::Threads)

add_test(NAME afl_tests COMMAND afl_tests)
