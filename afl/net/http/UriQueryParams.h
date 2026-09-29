/**
 * @file   UriQueryParams.h
 * @brief  URI 查询参数解析
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"
#include <string>
#include <vector>
namespace afl
{
namespace net
{
struct UriQueryParam
{
public:
    UriQueryParam() {}
    UriQueryParam(const std::string& name, const std::string& value) : m_name(name), m_value(value)
    {
    }

    void name(const std::string& n) { m_name = n; }
    void value(const std::string& v) { m_value = v; }
    const std::string name() const { return m_name; }
    const std::string value() const { return m_value; }

public:
    std::string m_name;
    std::string m_value;
};

class UriQueryParams
{
public:
    bool parse(const std::string& params);
    void appendToString(std::string* target) const;
    void writeToString(std::string* target) const;
    std::string toString() const;

    const UriQueryParam* find(const std::string& name) const;
    UriQueryParam* find(const std::string& name);

    UriQueryParam& get(size_t index);
    const UriQueryParam& get(size_t index) const;

    bool getValue(const std::string& name, std::string* value) const;
    bool getValue(const std::string& name, int* value) const;
    const std::string& getOrDefaultValue(const std::string& name,
                                         const std::string& default_value) const;

    size_t count() const;
    void clear();
    bool isEmpty() const;
    void add(const UriQueryParam& param);
    void add(const std::string& name, const std::string& value);
    void set(const std::string& name, const std::string& value);
    bool remove(const std::string& name);

private:
    std::vector<UriQueryParam> m_params;
};

} // namespace net
} // namespace afl
