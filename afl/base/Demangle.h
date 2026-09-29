/**
 * @file   Demangle.h
 * @brief  C++ 类型名 demangle 工具（RTTI 名称转可读名称）
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
namespace afl
{
namespace base
{
/**
 * @brief 将 Itanium ABI 修饰名还原为可读符号名
 * @return 成功为 true；失败为 false（unmangled 仍可能被改写）
 */
bool demangleName(const char* mangled, char* unmangled, size_t buf_size);
bool demangleName(const char* mangled, std::string& unmangled);

} // namespace base
} // namespace afl
