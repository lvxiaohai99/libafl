/**
 * @file   Demangle.cpp
 * @brief  C++ 类型名 demangle 的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/base/Demangle.h"
#include <cxxabi.h>

namespace afl
{
namespace base
{
bool demangleName(const char* mangled, char* unmangled, size_t buf_size)
{
    int status;

    static const size_t max_size = 1024;
    char temp[max_size];

    if (1 == sscanf(mangled, "%*[^(]%*[^_]%127[^)+]", temp))
    {
        unmangled = abi::__cxa_demangle(temp, unmangled, &buf_size, &status);
        if (status == 0)
        {
            //            printf("Name after  Mangled : %s ; Name before Mangled : %s\n", unmangled, temp);
            return true;
        }
    }

    //    printf("Name after  Mangled fail: %s ; Name before Mangled : %s\n", unmangled, temp);
    return false;
}

bool demangleName(const char* mangled, std::string& unmangled)
{
    static const size_t max_size = 1024;
    char result[max_size];
    if (demangleName(mangled, result, max_size))
    {
        unmangled = result;
        return true;
    }
    return false;
}

} // namespace base
} // namespace afl
