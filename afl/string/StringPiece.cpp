/**
 * @file   StringPiece.cpp
 * @brief  字符串片段的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/string/StringPiece.h"
#include <limits.h> // for UCHAR_MAX
#include <iostream>
#include <algorithm>
namespace afl
{
namespace str
{
typedef StringPiece::size_type size_type;

size_type StringPiece::find(const StringPiece& s, size_type pos) const
{
    if (pos > m_length)
        return npos;

    const char* result = std::search(m_ptr + pos, m_ptr + m_length, s.m_ptr, s.m_ptr + s.m_length);
    const size_type xpos = result - m_ptr;
    return xpos + s.m_length <= m_length ? xpos : npos;
}

size_type StringPiece::find(char c, size_type pos) const
{
    if (pos >= m_length)
        return npos;

    const void* result = memchr(m_ptr + pos, c, m_length - pos);
    if (result)
        return reinterpret_cast<const char*>(result) - m_ptr;
    return npos;
}

size_type StringPiece::rfind(const StringPiece& s, size_type pos) const
{
    if (m_length < s.m_length)
        return npos;

    if (s.empty())
        return std::min(m_length, pos);

    const char* last = m_ptr + std::min(m_length - s.m_length, pos) + s.m_length;
    const char* result = std::find_end(m_ptr, last, s.m_ptr, s.m_ptr + s.m_length);
    return result != last ? static_cast<size_t>(result - m_ptr) : npos;
}

size_type StringPiece::rfind(char c, size_type pos) const
{
    if (m_length == 0)
        return npos;

    for (size_type i = std::min(pos, m_length - 1);; --i)
    {
        if (m_ptr[i] == c)
            return i;
        if (i == 0)
            break;
    }
    return npos;
}

// 对 characters_wanted 中每个字符，在 table 中把对应 ASCII 下标置 1，供 find*Of  O(1) 查表
// table 须能覆盖 unsigned char 全部取值，例如：bool table[UCHAR_MAX + 1]
static inline void BuildLookupTable(const StringPiece& characters_wanted, bool* table)
{
    const size_type length = characters_wanted.length();
    const char* const data = characters_wanted.data();
    for (size_type i = 0; i < length; ++i)
    {
        table[static_cast<unsigned char>(data[i])] = true;
    }
}

size_type StringPiece::findFirstOf(const StringPiece& s, size_type pos) const
{
    if (m_length == 0 || s.m_length == 0)
        return npos;

    // 单字符查找无需构建查表
    if (s.m_length == 1)
        return findFirstOf(s.m_ptr[0], pos);

    bool lookup[UCHAR_MAX + 1] = {false};
    BuildLookupTable(s, lookup);
    for (size_type i = pos; i < m_length; ++i)
    {
        if (lookup[static_cast<unsigned char>(m_ptr[i])])
        {
            return i;
        }
    }
    return npos;
}
size_type StringPiece::findFirstNotOf(const StringPiece& s, size_type pos) const
{
    if (m_length == 0)
        return npos;

    if (s.m_length == 0)
        return 0;

    // 单字符查找无需构建查表
    if (s.m_length == 1)
        return findFirstNotOf(s.m_ptr[0], pos);

    bool lookup[UCHAR_MAX + 1] = {false};
    BuildLookupTable(s, lookup);
    for (size_type i = pos; i < m_length; ++i)
    {
        if (!lookup[static_cast<unsigned char>(m_ptr[i])])
        {
            return i;
        }
    }
    return npos;
}

size_type StringPiece::findFirstNotOf(char c, size_type pos) const
{
    if (m_length == 0)
        return npos;

    for (; pos < m_length; ++pos)
    {
        if (m_ptr[pos] != c)
        {
            return pos;
        }
    }
    return npos;
}

size_type StringPiece::findLastOf(const StringPiece& s, size_type pos) const
{
    if (m_length == 0 || s.m_length == 0)
        return npos;

    // 单字符查找无需构建查表
    if (s.m_length == 1)
        return findLastOf(s.m_ptr[0], pos);

    bool lookup[UCHAR_MAX + 1] = {false};
    BuildLookupTable(s, lookup);
    for (size_type i = std::min(pos, m_length - 1);; --i)
    {
        if (lookup[static_cast<unsigned char>(m_ptr[i])])
            return i;
        if (i == 0)
            break;
    }
    return npos;
}

size_type StringPiece::findLastNotOf(const StringPiece& s, size_type pos) const
{
    if (m_length == 0)
        return npos;

    size_type i = std::min(pos, m_length - 1);
    if (s.m_length == 0)
        return i;

    // 单字符查找无需构建查表
    if (s.m_length == 1)
        return findLastNotOf(s.m_ptr[0], pos);

    bool lookup[UCHAR_MAX + 1] = {false};
    BuildLookupTable(s, lookup);
    for (;; --i)
    {
        if (!lookup[static_cast<unsigned char>(m_ptr[i])])
            return i;
        if (i == 0)
            break;
    }
    return npos;
}

size_type StringPiece::findLastNotOf(char c, size_type pos) const
{
    if (m_length == 0)
        return npos;

    for (size_type i = std::min(pos, m_length - 1);; --i)
    {
        if (m_ptr[i] != c)
            return i;
        if (i == 0)
            break;
    }
    return npos;
}

/* Like memcmp, but ignore differences in case.
Convert to upper case (not lower) before comparing so that
join -i works with sort -f.  */
int memcasecmp(const void* vs1, const void* vs2, size_t n)
{
    char const* s1 = static_cast<char const*>(vs1);
    char const* s2 = static_cast<char const*>(vs2);
    for (size_t i = 0; i < n; i++)
    {
        unsigned char u1 = s1[i];
        unsigned char u2 = s2[i];
        int U1 = toupper(u1);
        int U2 = toupper(u2);
        int diff = (UCHAR_MAX <= INT_MAX ? U1 - U2 : U1 < U2 ? -1 : U2 < U1);
        if (diff)
            return diff;
    }
    return 0;
}

int StringPiece::ignore_case_compare(const StringPiece& x) const
{
    int r = memcasecmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length);
    if (r != 0)
        return r;
    if (m_length < x.m_length)
        return -1;
    else if (m_length > x.m_length)
        return 1;
    return 0;
}

bool StringPiece::ignore_case_equal(const StringPiece& other) const
{
    return size() == other.size() && memcasecmp(data(), other.data(), size()) == 0;
}

} // namespace str
} // namespace afl

std::ostream& operator<<(std::ostream& o, const afl::str::StringPiece& piece)
{
    return (o << piece.asString());
}
