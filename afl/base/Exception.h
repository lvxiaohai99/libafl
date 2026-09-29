/**
 * @file   Exception.h
 * @brief  异常基类封装
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <exception>
namespace afl
{
namespace base
{
class Exception : public std::exception
{
public:
    explicit Exception(const char* errinfo);
    Exception(const char* filename, int linenumber, const char* errinfo);
    Exception(const char* filename, int linenumber, const std::string& errinfo);
    virtual ~Exception() throw();

    virtual const char* what() const throw() { return m_errmsg.c_str(); }

    const char* stackTrace() const throw() { return m_callStack.c_str(); }

    const char* filename() const throw() { return m_filename.c_str(); }

    int line() const throw() { return m_line; }

private:
    void traceStack();

    int m_line;
    std::string m_filename;
    std::string m_errmsg;
    std::string m_callStack;
};

} // namespace base
} // namespace afl
