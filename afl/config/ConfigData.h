/**
 * @file   ConfigData.h
 * @brief  可序列化配置数据基类与 CRTP 模板
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/SerializableData.h"
#include "afl/config/ConfigFile.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace afl
{
namespace config
{
/**
 * @brief 配置数据抽象基类（按文件名归属）
 */
class ConfigDataBase : public afl::base::SerializableData
{
public:
    virtual ~ConfigDataBase() {}

    /**
     * @brief 从配置文件块重载到本对象
     * @param name 配置键名
     * @param cf   所属配置文件
     */
    virtual void reload(std::string name, std::shared_ptr<ConfigFile> cf) = 0;

    /**
     * @brief 将本对象写回配置文件块并保存
     * @param name 配置键名
     * @param cf   所属配置文件
     */
    virtual void save(std::string name, std::shared_ptr<ConfigFile> cf) = 0;

    /** @brief 关联的配置文件路径 */
    virtual const std::string& getFileName() const { return m_fileName; }

    /**
     * @brief 设置关联文件路径
     * @param fileName 路径
     */
    virtual void setFileName(const std::string& fileName) { m_fileName = fileName; }

private:
    std::string m_fileName;
};

/**
 * @brief 配置数据 CRTP 模板；派生类实现 writeToFile / readFromFile
 * @tparam T 派生类型，须继承 ConfigData&lt;T&gt;
 */
template <typename T>
class ConfigData : public ConfigDataBase
{
public:
    using UpdateCallback = std::function<void(T& data)>;

    /**
     * @brief 构造
     * @param comment JSON 中可选 "_comment" 字段
     * @param cb      重载成功后的回调
     */
    explicit ConfigData(std::string comment = "", UpdateCallback cb = nullptr)
        : m_comment(std::move(comment)), m_callback(std::move(cb))
    {
    }

    /**
     * @brief 保存到配置文件
     * @param name 键名
     * @param cf   配置文件
     */
    virtual void save(std::string name, std::shared_ptr<ConfigFile> cf)
    {
        cf->configBlock(name) = *(static_cast<T*>(this));
        cf->save();
    }

    /**
     * @brief 从配置文件重载
     * @param name 键名
     * @param cf   配置文件
     */
    virtual void reload(std::string name, std::shared_ptr<ConfigFile> cf)
    {
        try
        {
            auto& block = cf->configBlock(name);
            T newData = block;
            newData.setUpdateCallback(this->m_callback);
            newData.setFileName(this->getFileName());
            *(static_cast<T*>(this)) = newData;
            if (m_callback)
            {
                m_callback(*(static_cast<T*>(this)));
            }
            block = *(static_cast<T*>(this));
        }
        catch (const ConfigBlock::exception& e)
        {
            cf->configBlock(name) = *(static_cast<T*>(this));
            std::cerr << "ConfigData::reload failed: " << e.what() << std::endl;
        }
    }

    /**
     * @brief 设置更新回调
     * @param cb 回调
     */
    void setUpdateCallback(UpdateCallback cb) { m_callback = std::move(cb); }

    /** @brief 获取更新回调 */
    UpdateCallback& getUpdateCallback() { return m_callback; }

private:
    virtual void serialize(nlohmann::json& j)
    {
        writeToFile(static_cast<ConfigBlock&>(j));
        if (!m_comment.empty())
        {
            j["_comment"] = m_comment;
        }
    }

    virtual void deserialize(const nlohmann::json& j)
    {
        readFromFile(static_cast<const ConfigBlock&>(j));
    }

    /** @brief 派生类：写入 JSON 块 */
    virtual void writeToFile(ConfigBlock& block) = 0;
    /** @brief 派生类：从 JSON 块读取 */
    virtual void readFromFile(const ConfigBlock& block) = 0;

    std::string m_comment;
    UpdateCallback m_callback;
};

} // namespace config
} // namespace afl
