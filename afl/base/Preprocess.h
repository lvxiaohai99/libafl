/**
 * @file   Preprocess.h
 * @brief  预处理/宏工具
 * @author libafl
 * @date   2026-09
 */
#pragma once
/** @brief 连接两个预处理记号 */
#define AFL_PP_JOIN(X, Y) AFL_PP_DO_JOIN(X, Y)
#define AFL_PP_DO_JOIN(X, Y) AFL_PP_DO_JOIN2(X, Y)
#define AFL_PP_DO_JOIN2(X, Y) X##Y

/** @brief 宏展开后将 X 转为字符串字面量，例：AFL_PP_STRINGIZE(UCHAR_MAX) -> "255" */
#define AFL_PP_STRINGIZE(X) AFL_PP_DO_STRINGIZE(X)
#define AFL_PP_DO_STRINGIZE(X) #X
