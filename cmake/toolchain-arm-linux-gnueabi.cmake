# arm-linux-gnueabi 交叉编译工具链（gcc-linaro 4.9.4）
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(AFL_TOOLCHAIN_DIR "/home/lvhaitao/v2x/cross_tool_chain/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi"
    CACHE PATH "cross toolchain root")

set(CMAKE_C_COMPILER   ${AFL_TOOLCHAIN_DIR}/bin/arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER ${AFL_TOOLCHAIN_DIR}/bin/arm-linux-gnueabi-g++)
set(CMAKE_AR           ${AFL_TOOLCHAIN_DIR}/bin/arm-linux-gnueabi-ar)
set(CMAKE_RANLIB       ${AFL_TOOLCHAIN_DIR}/bin/arm-linux-gnueabi-ranlib)

set(CMAKE_FIND_ROOT_PATH ${AFL_TOOLCHAIN_DIR})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
