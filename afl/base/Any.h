/**
 * @file   Any.h
 * @brief  Any 类型擦除容器（类 boost::any）
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

namespace afl
{
namespace base
{
class any
{
public:
    any() : m_content(0) {}

    any(const any& that) : m_content(that.clone()) {}

    any(any&& that) : m_content(std::move(that.m_content)) {}

    template <typename U>
    any(U&& value) : m_content(new holder<typename std::decay<U>::type>(std::forward<U>(value)))
    {
    }

    any& operator=(const any& rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    template <typename ValueType>
    any& operator=(const ValueType& rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    ~any() { delete m_content; }

public:
    any& swap(any& rhs)
    {
        std::swap(m_content, rhs.m_content);
        return *this;
    }

    bool empty() const { return !m_content; }

    void clear() { any().swap(*this); }

    const std::type_info& type() const { return m_content ? m_content->type() : typeid(void); }

private:
    struct placeholder
    {
        virtual ~placeholder() {}
        virtual const std::type_info& type() const = 0;
        virtual placeholder* clone() const = 0;
    };

    template <typename ValueType>
    class holder : public placeholder
    {
    public:
        holder(const ValueType& value) : m_held(value) {}

    public:
        virtual const std::type_info& type() const { return typeid(ValueType); }

        virtual placeholder* clone() const { return new holder(m_held); }

    public:
        ValueType m_held;

    private:
        holder& operator=(const holder&);
    };

    placeholder* clone() const
    {
        if (m_content != 0)
            return m_content->clone();

        return 0;
    }

private:
    template <typename ValueType>
    friend ValueType* any_cast(any*);

private:
    placeholder* m_content;
};

inline void swap(any& lhs, any& rhs)
{
    lhs.swap(rhs);
}

class bad_any_cast : public std::bad_cast
{
public:
    virtual const char* what() const throw()
    {
        return "afl::bad_any_cast: failed conversion using afl::any_cast";
    }
};

template <typename ValueType>
inline ValueType* any_cast(any* operand)
{
    return operand && operand->type() == typeid(ValueType)
               ? &static_cast<any::holder<ValueType>*>(operand->m_content)->m_held
               : 0;
}

template <typename ValueType>
inline const ValueType* any_cast(const any* operand)
{
    return afl::base::any_cast<ValueType>(const_cast<any*>(operand));
}

template <typename ValueType>
inline ValueType any_cast(any& operand)
{
    typedef ValueType nonref;

    nonref* result = any_cast<nonref>(&operand);
    if (!result)
        throw bad_any_cast();

    return static_cast<ValueType>(*result);
}

template <typename ValueType>
inline ValueType any_cast(const any& operand)
{
    typedef ValueType nonref;
    return any_cast<const nonref>(const_cast<any&>(operand));
}

} // namespace base
} // namespace afl
