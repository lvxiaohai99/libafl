/**
 * @file   StringPiece.h
 * @brief  字符串片段视图（类 string_view）
 * @author libafl
 * @date   2026-09
 */
#pragma once
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// 指向外部内存的轻量字符串视图，便于同时接受 const char* 与 std::string。
#include "afl/base/Common.h"
#include <string.h>
#include <stddef.h> // for ptrdiff_t
#include <assert.h>
#include <string>
#include <iosfwd> // for ostream forward-declaration

#if 0
#define HAVE_TYPE_TRAITS
#include <type_traits.h>
#elif 0
#define HAVE_TYPE_TRAITS
#include <bits/type_traits.h>
#endif

//using std::memcmp;
//using std::strlen;
using std::string;

// 参考 pcre_stringpiece.h、leveldb::Slice：仅保存外部指针与长度。
// 使用者须保证底层存储生命周期长于 StringPiece；底层释放后不得再使用。
// 多线程只读 const 接口无需加锁；若存在非 const 访问，须外部同步。
namespace afl
{
namespace str
{
class StringPiece
{
private:
    const char* m_ptr;
    size_t m_length;

public:
    // standard STL container boilerplate
    typedef size_t size_type;
    typedef char value_type;
    typedef const char* pointer;
    typedef const char& reference;
    typedef const char& const_reference;
    typedef ptrdiff_t difference_type;
    typedef const char* const_iterator;
    typedef const char* iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    static const size_type npos = ~size_type(0);

public:
    // 提供隐式转换构造，便于在需要 StringPiece 处直接传 const char* 或 string
    StringPiece() : m_ptr(NULL), m_length(0) {}
    StringPiece(const char* str) : m_ptr(str), m_length(strlen(m_ptr)) {}
    StringPiece(const unsigned char* str)
        : m_ptr(reinterpret_cast<const char*>(str)), m_length(static_cast<int>(strlen(m_ptr)))
    {
    }
    StringPiece(const string& str) : m_ptr(str.data()), m_length(str.size()) {}
    StringPiece(const char* offset, int len) : m_ptr(offset), m_length(len) {}

    // data() 可能指向含内嵌 '\0' 的缓冲区，且未必以 NUL 结尾；勿当作 C 字符串传给依赖 NUL 结尾的 API
    // 若确需 C 字符串，请用 asString().c_str()，或改造调用方不依赖 NUL 结尾
    const char* data() const { return m_ptr; }
    size_t size() const { return m_length; }
    size_t length() const { return m_length; }
    bool empty() const { return m_length == 0; }
    void clear()
    {
        m_ptr = NULL;
        m_length = 0;
    }
    iterator begin() const { return m_ptr; }
    iterator end() const { return m_ptr + m_length; }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(m_ptr + m_length); }
    const_reverse_iterator rend() const { return const_reverse_iterator(m_ptr); }

    size_type max_size() const { return m_length; }
    size_type capacity() const { return m_length; }

    void set(const char* buffer, int len)
    {
        m_ptr = buffer;
        m_length = len;
    }
    void set(const char* str)
    {
        m_ptr = str;
        m_length = str ? strlen(str) : 0;
    }
    void set(const void* buffer, int len)
    {
        m_ptr = reinterpret_cast<const char*>(buffer);
        m_length = len;
    }

    char operator[](int i) const { return m_ptr[i]; }

    void removePrefix(int n)
    {
        m_ptr += n;
        m_length -= n;
    }

    void removeSuffix(int n) { m_length -= n; }

    bool operator==(const StringPiece& x) const
    {
        return ((m_length == x.m_length) && (memcmp(m_ptr, x.m_ptr, m_length) == 0));
    }
    bool operator!=(const StringPiece& x) const { return !(*this == x); }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                                                  \
    bool operator cmp(const StringPiece& x) const                                                  \
    {                                                                                              \
        int r = memcmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length);             \
        return ((r auxcmp 0) || ((r == 0) && (m_length cmp x.m_length)));                          \
    }

    STRINGPIECE_BINARY_PREDICATE(<, <)
    STRINGPIECE_BINARY_PREDICATE(<=, <)
    STRINGPIECE_BINARY_PREDICATE(>=, >)
    STRINGPIECE_BINARY_PREDICATE(>, >)
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece& x) const
    {
        int r = memcmp(m_ptr, x.m_ptr, m_length < x.m_length ? m_length : x.m_length);
        if (r == 0)
        {
            if (m_length < x.m_length)
                r = -1;
            else if (m_length > x.m_length)
                r = +1;
        }
        return r;
    }
    int ignore_case_compare(const StringPiece& other) const;
    bool ignore_case_equal(const StringPiece& other) const;

    string asString() const { return string(data(), size()); }
    void copy_to_string(string* target) const { target->assign(m_ptr, m_length); }
    void append_to_string(std::string* target) const
    {
        if (!empty())
            target->append(data(), size());
    }

    /** @brief 是否以 x 为前缀 */
    bool startsWith(const StringPiece& x) const
    {
        return ((m_length >= x.m_length) && (memcmp(m_ptr, x.m_ptr, x.m_length) == 0));
    }

    /** @brief 是否以 x 为后缀 */
    bool endsWith(const StringPiece& x) const
    {
        return ((m_length >= x.m_length) && (memcmp(m_ptr, x.m_ptr, x.m_length) == 0));
    }

    size_type find(const StringPiece& s, size_type pos = 0) const;
    size_type find(char c, size_type pos = 0) const;
    size_type rfind(const StringPiece& s, size_type pos = npos) const;
    size_type rfind(char c, size_type pos = npos) const;

    size_type findFirstOf(const StringPiece& s, size_type pos = 0) const;
    size_type findFirstOf(char c, size_type pos = 0) const { return find(c, pos); }
    size_type findFirstNotOf(const StringPiece& s, size_type pos = 0) const;
    size_type findFirstNotOf(char c, size_type pos = 0) const;
    size_type findLastOf(const StringPiece& s, size_type pos = npos) const;
    size_type findLastOf(char c, size_type pos = npos) const { return rfind(c, pos); }
    size_type findLastNotOf(const StringPiece& s, size_type pos = npos) const;
    size_type findLastNotOf(char c, size_type pos = npos) const;

    StringPiece substr(size_type pos, size_type n = npos) const
    {
        if (pos > m_length)
            pos = m_length;
        if (n > m_length - pos)
            n = m_length - pos;
        return StringPiece(m_ptr + pos, n);
    }
};

} // namespace str
} // namespace afl

// ------------------------------------------------------------------
// 在 STL 容器中存放 StringPiece 时，须保证底层 string/char* 生命周期长于 StringPiece
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// 便于部分 STL 实现将 vector<StringPiece> 视为 POD 容器优化
template <>
struct __type_traits<StringPiece>
{
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const afl::str::StringPiece& piece);
