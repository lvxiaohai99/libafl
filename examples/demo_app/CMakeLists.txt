# libafl 综合示例：配置 / 日志 / Module / EventLoop / Signal
add_executable(afl_demo_app
    main.cpp
)

target_include_directories(afl_demo_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(afl_demo_app PRIVATE afl::afl)
target_compile_options(afl_demo_app PRIVATE -Wall)

# 把 conf/ 拷到可执行文件同目录，方便默认路径 ./conf
add_custom_command(TARGET afl_demo_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/conf
            $<TARGET_FILE_DIR:afl_demo_app>/conf
    COMMENT "copy demo_app conf/"
)

set_target_properties(afl_demo_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS afl_demo_app
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# 安装后可执行文件旁仍有 conf/，与 POST_BUILD 行为一致
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/conf
        DESTINATION ${CMAKE_INSTALL_BINDIR})
