/**
 * @file   NonCopy.h
 * @brief  禁止拷贝构造与赋值，建议以 private 方式继承
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

namespace afl
{
namespace base
{
class NonCopy
{
protected:
    NonCopy() {}
    ~NonCopy() {}

private:
    NonCopy(const NonCopy&);
    const NonCopy& operator=(const NonCopy&);
};

} // namespace base
} // namespace afl
