/**
 * @file   UriQueryParams.cpp
 * @brief  URI 查询参数解析的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/net/http/UriQueryParams.h"
#include "afl/string/StringUtil.h"
#include <algorithm>
namespace afl
{
namespace net
{
bool UriQueryParams::parse(const std::string& params)
{
    clear();
    std::vector<std::string> splited;
    afl::str::split(params, splited, "&");
    for (size_t i = 0; i < splited.size(); ++i)
    {
        m_params.push_back(UriQueryParam());
        size_t pos = splited[i].find('=');
        if (pos != std::string::npos)
        {
            m_params.back().m_name.assign(splited[i], 0, pos);
            m_params.back().m_value.assign(splited[i], pos + 1, std::string::npos);
        }
        else
        {
            m_params.back().m_name = splited[i];
        }
    }
    return true;
}

void UriQueryParams::appendToString(std::string* target) const
{
    for (size_t i = 0; i < m_params.size(); ++i)
    {
        if (!m_params[i].m_name.empty())
        {
            const UriQueryParam& param = m_params[i];
            target->append(param.m_name);
            target->push_back('=');
            target->append(param.m_value);
            if (i != m_params.size() - 1)
                target->push_back('&');
        }
    }
}

void UriQueryParams::writeToString(std::string* target) const
{
    target->clear();
    appendToString(target);
}

std::string UriQueryParams::toString() const
{
    std::string result;
    appendToString(&result);
    return result;
}

UriQueryParam* UriQueryParams::find(const std::string& name)
{
    for (size_t i = 0; i < m_params.size(); ++i)
    {
        if (m_params[i].m_name == name)
            return &m_params[i];
    }
    return NULL;
}

const UriQueryParam* UriQueryParams::find(const std::string& name) const
{
    return const_cast<UriQueryParams*>(this)->find(name);
}

UriQueryParam& UriQueryParams::get(size_t index)
{
    return m_params.at(index);
}

const UriQueryParam& UriQueryParams::get(size_t index) const
{
    return m_params.at(index);
}

bool UriQueryParams::getValue(const std::string& name, std::string* value) const
{
    const UriQueryParam* param = find(name);
    if (param)
    {
        *value = param->m_value;
        return true;
    }
    return false;
}

bool UriQueryParams::getValue(const std::string& name, int* value) const
{
    const UriQueryParam* param = find(name);
    if (param)
    {
        *value = atoi(param->m_value.c_str());
        return true;
    }
    return false;
}

const std::string& UriQueryParams::getOrDefaultValue(const std::string& name,
                                                     const std::string& default_value) const
{
    const UriQueryParam* param = find(name);
    if (param)
    {
        return param->m_value;
    }
    return default_value;
}

size_t UriQueryParams::count() const
{
    return m_params.size();
}

void UriQueryParams::clear()
{
    m_params.clear();
}

void UriQueryParams::add(const UriQueryParam& param)
{
    m_params.push_back(param);
}

void UriQueryParams::add(const std::string& name, const std::string& value)
{
    m_params.push_back(UriQueryParam());
    m_params.back().m_name = name;
    m_params.back().m_value = value;
}

void UriQueryParams::set(const std::string& name, const std::string& value)
{
    UriQueryParam* param = find(name);
    if (param)
        param->m_value = value;
    else
        add(name, value);
}

bool UriQueryParams::remove(const std::string& name)
{
    std::vector<UriQueryParam>::iterator iter;
    for (iter = m_params.begin(); iter != m_params.end(); ++iter)
    {
        if ((*iter).m_name == name)
        {
            m_params.erase(iter);
            return true;
        }
    }
    return false;
}

} // namespace net
} // namespace afl
