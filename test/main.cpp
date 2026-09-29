/**
 * @file   main.cpp
 * @brief  libafl 单元测试入口
 * @author libafl
 * @date   2026-09
 */
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
