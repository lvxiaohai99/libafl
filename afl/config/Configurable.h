/**
 * @file   Configurable.h
 * @brief  可配置对象模板：绑定 ConfigManager 与配置数据结构
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/config/ConfigManager.h"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace afl
{
namespace config
{
/**
 * @brief 将配置数据 T 注册到 ConfigManager 的 RAII 包装
 * @tparam T 必须派生自 ConfigData&lt;T&gt;
 */
template <typename T>
class Configurable
{
    static_assert(std::is_base_of<ConfigData<T>, T>::value,
                  "Template type T must derive from ConfigData<T>");

public:
    using ConfigType = T;

    /**
     * @brief 注册配置
     * @param cm   配置管理器
     * @param key  配置键
     * @param file 配置文件名（空则用默认文件）
     */
    Configurable(ConfigManager& cm, std::string key, std::string file = std::string())
        : m_configManager(cm), m_key(std::move(key)), m_handle(0)
    {
        m_handle = m_configManager.registerConfig(m_key, m_data, file);
    }

    virtual ~Configurable() { m_configManager.unregisterConfig(m_handle); }

    /** @brief 获取配置数据引用 */
    ConfigType& getConfig() { return m_data; }

    /**
     * @brief 用新配置覆盖（保留文件名与回调）
     * @param config 新配置
     */
    void setConfig(ConfigType& config)
    {
        config.setFileName(m_data.getFileName());
        config.setUpdateCallback(m_data.getUpdateCallback());
        m_data = config;
    }

    /** @brief 持久化当前配置到关联文件 */
    void saveConfig()
    {
        auto cf = m_configManager.getConfigFile(m_handle);
        if (cf)
        {
            m_data.save(m_key, cf);
        }
    }

private:
    ConfigManager& m_configManager;
    std::string m_key;
    std::uint64_t m_handle;
    T m_data;
};

} // namespace config
} // namespace afl
