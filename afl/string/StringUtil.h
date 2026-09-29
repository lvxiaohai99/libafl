/**
 * @file   StringUtil.h
 * @brief  字符串辅助函数
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>

namespace afl
{
namespace str
{
size_t stringFormatAppend(std::string* dst, const char* format, ...);
size_t stringFormat(std::string* dst, const char* format, ...);
std::string stringFormat(const char* format, ...);

template <typename T>
inline std::string toStr(const T& t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

template <typename T>
T strTo(const std::string& str)
{
    T t;
    std::istringstream iss(str);
    iss >> t;
    return t;
}

/** @brief 忽略大小写的字符串比较，可用作 map/set 的比较器 */
struct StringCmpNocase : public std::binary_function<std::string, std::string, bool>
{
    bool operator()(const std::string& lhs, const std::string& rhs) const
    {
        std::string::const_iterator p = lhs.begin();
        std::string::const_iterator p2 = rhs.begin();

        while (p != lhs.end() && p2 != rhs.end())
        {
            if (toupper(*p) != toupper(*p2))
            {
                return (toupper(*p) < toupper(*p2) ? 1 : 0);
            }
            ++p;
            ++p2;
        }

        return (lhs.size() == rhs.size()) ? 0 : (lhs.size() < rhs.size()) ? 1 : 0;
    }
};

inline std::string toLower(const std::string& str)
{
    std::string t = str;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    return t;
}

inline std::string toUpper(const std::string& str)
{
    std::string t = str;
    std::transform(t.begin(), t.end(), t.begin(), ::toupper);
    return t;
}

inline bool startsWith(const std::string& str, const std::string& substr)
{
    return str.find(substr) == 0;
}

inline bool endsWith(const std::string& str, const std::string& substr)
{
    return str.rfind(substr) == (str.length() - substr.length());
}

inline bool equals(const std::string& lhs, const std::string& rhs)
{
    return (lhs) == (rhs);
}

inline std::string& trimLeft(std::string& str, const char* delim = " ")
{
    str.erase(0, str.find_first_not_of(delim));
    return str;
}

inline std::string& trimRight(std::string& str, const char* delim = " ")
{
    str.erase(str.find_last_not_of(delim) + 1);
    return str;
}

inline std::string& trim(std::string& str, const char* delim = " ")
{
    trimLeft(str, delim);
    trimRight(str, delim);
    return str;
}

inline std::string erase(std::string& str, char c = ' ')
{
    str.erase(std::remove_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), c)),
              str.end());
    return str;
}

inline std::string replaceAll(std::string& str, const char* delim, const char* s = "")
{
    size_t len = strlen(delim);
    size_t pos = str.find(delim);
    while (pos != std::string::npos)
    {
        str.replace(pos, len, s);
        pos = str.find(delim, pos);
    }
    return str;
}

inline void split(const std::string& str, std::vector<std::string>& result,
                  const std::string& delim = " ", bool insertEmpty = false)
{
    if (str.empty() || delim.empty())
    {
        return;
    }

    std::string::const_iterator substart = str.begin(), subend;
    while (true)
    {
        subend = std::search(substart, str.end(), delim.begin(), delim.end());
        std::string temp(substart, subend);

        if (!temp.empty())
        {
            result.push_back(temp);
        }
        else if (insertEmpty)
        {
            result.push_back("");
        }

        if (subend == str.end())
        {
            break;
        }
        substart = subend + delim.size();
    }
}

/** @brief 用分隔符连接序列中的字符串 */
template <typename SequenceSequenceT, typename Range1T>
inline typename SequenceSequenceT::value_type join(const SequenceSequenceT& Input,
                                                   const Range1T& Separator)
{
    typedef typename SequenceSequenceT::value_type ResultT;
    typedef typename SequenceSequenceT::const_iterator InputIteratorT;

    InputIteratorT itBegin = Input.begin();
    InputIteratorT itEnd = Input.end();

    ResultT Result;

    if (itBegin != itEnd)
    {
        Result += *itBegin;
        ++itBegin;
    }

    for (; itBegin != itEnd; ++itBegin)
    {
        Result += Separator;
        Result += *itBegin;
    }

    return Result;
}

template <typename SequenceSequenceT, typename Range1T, typename PredicateT>
inline typename SequenceSequenceT::value_type join_if(const SequenceSequenceT& Input,
                                                      const Range1T& Separator, PredicateT Pred)
{
    typedef typename SequenceSequenceT::value_type ResultT;
    typedef typename SequenceSequenceT::const_iterator InputIteratorT;

    InputIteratorT itBegin = Input.begin();
    InputIteratorT itEnd = Input.end();

    ResultT Result;

    while (itBegin != itEnd && !Pred(*itBegin))
    {
        ++itBegin;
    }

    if (itBegin != itEnd)
    {
        Result += *itBegin;
        ++itBegin;
    }

    for (; itBegin != itEnd; ++itBegin)
    {
        if (Pred(*itBegin))
        {
            Result += Separator;
            Result += *itBegin;
        }
    }

    return Result;
}

} // namespace str
} // namespace afl
