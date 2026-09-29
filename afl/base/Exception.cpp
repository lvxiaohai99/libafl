/**
 * @file   Exception.cpp
 * @brief  异常基类的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/base/Exception.h"
#include <stdlib.h>

#include <execinfo.h>
#define DO_NAME_DEMANGLE

#ifdef DO_NAME_DEMANGLE
#include "afl/base/Demangle.h"
#endif

namespace afl
{
namespace base
{
Exception::Exception(const char* errinfo)
    : m_line(0), m_filename("unknown filename"), m_errmsg(errinfo)
{
    traceStack();
}

Exception::Exception(const char* filename, int linenumber, const char* errinfo)
    : m_line(linenumber), m_filename(filename), m_errmsg(errinfo)
{
    traceStack();
}

Exception::Exception(const char* filename, int linenumber, const std::string& errinfo)
    : m_line(linenumber), m_filename(filename), m_errmsg(errinfo)
{
    traceStack();
}

Exception::~Exception() throw() {}

void Exception::traceStack()
{
    static const int len = 256;
    void* buffer[len];
    int nptrs = ::backtrace(buffer, len);
    char** strings = ::backtrace_symbols(buffer, nptrs);
    if (!strings)
        return;

    for (int i = 0; i < nptrs; ++i)
    {
#ifndef DO_NAME_DEMANGLE
        m_callStack.append(strings[i]);
#else
        std::string line(strings[i]); // ./test(_ZN6detail12c_do_nothingEfi+0x44) [0x401974]

        std::string unmangle;
        if (afl::base::demangleName(line.c_str(), unmangle))
        {
            m_callStack.append(unmangle);
        }
        else
        {
            m_callStack.append(strings[i]);
        }
#endif
        m_callStack.push_back('\n');
    }
    free(strings);
}

} // namespace base
} // namespace afl
