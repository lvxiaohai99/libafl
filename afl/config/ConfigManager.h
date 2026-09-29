/**
 * @file   ConfigManager.h
 * @brief  多配置文件 / 多配置块注册与热重载管理
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/config/ConfigData.h"
#include "afl/file/FileUtil.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

namespace afl
{
namespace config
{
/**
 * @brief 配置管理器：维护 文件路径 ↔ ConfigFile ↔ 已注册 ConfigData
 *
 * 注册句柄为 ConfigDataBase* 的 uintptr 表示，仅在进程内有效。
 */
class ConfigManager final
{
public:
    /**
     * @brief 构造
     * @param root 配置根目录（自动追加 '/'）；默认文件为 root/default.cfg
     */
    explicit ConfigManager(std::string root = "./");
    ~ConfigManager();

    /** @brief 将所有已注册配置写回各自文件 */
    void saveConfig();

    /** @brief 重载根目录下全部已打开配置文件 */
    void reloadConfig();

    /**
     * @brief 重载指定文件并刷新绑定到该文件的配置对象
     * @param path 配置文件路径（与 register 时一致）
     * @throws std::logic_error 文件未登记
     */
    void reloadConfig(std::string path);

    /**
     * @brief 取消注册
     * @param handle registerConfig 返回的句柄
     */
    void unregisterConfig(std::uint64_t handle);

    /**
     * @brief 按句柄取配置文件
     * @param handle 注册句柄
     * @return 文件智能指针；找不到返回 nullptr
     */
    std::shared_ptr<ConfigFile> getConfigFile(std::uint64_t handle);

    /**
     * @brief 注册配置对象
     * @tparam T 派生自 ConfigDataBase
     * @param key  配置块键名
     * @param data 配置对象引用（生命周期须覆盖注销前）
     * @param file 相对 root 的文件名或绝对路径；空则用 default.cfg
     * @return 句柄（用于 unregister / getConfigFile / save）
     */
    template <typename T>
    std::uint64_t registerConfig(std::string key, T& data, std::string file = std::string())
    {
        static_assert(std::is_base_of<ConfigDataBase, T>::value,
                      "T must derive from ConfigDataBase");

        if (file.empty())
        {
            file = m_defaultConfigFile->fileName();
        }
        else if (file[0] != '/')
        {
            file = m_rootDir + file;
        }

        if (m_pathToFile.find(file) == m_pathToFile.end())
        {
            afl::file::FileUtil::createRecursionDir(
                afl::file::FileUtil::dirName(file.c_str()).c_str());
            m_pathToFile[file] = std::shared_ptr<ConfigFile>(new ConfigFile(file));
        }

        auto& block = m_pathToFile[file]->configBlock(key);
        if (block.is_null())
        {
            try
            {
                block = data;
            }
            catch (const ConfigBlock::exception& e)
            {
                std::cerr << "ConfigManager: set default failed: " << e.what() << std::endl;
            }
        }
        else
        {
            try
            {
                data = block;
            }
            catch (const ConfigBlock::exception& e)
            {
                std::cerr << "ConfigManager: load existing failed: " << e.what() << std::endl;
            }
        }

        m_pathToFile[file]->save();

        if (m_fileToModules.find(m_pathToFile[file]) == m_fileToModules.end())
        {
            m_fileToModules[m_pathToFile[file]] = std::map<ConfigDataBase*, std::string>();
        }

        ConfigDataBase* base = &data;
        base->setFileName(file);
        m_fileToModules[m_pathToFile[file]][base] = key;

        return reinterpret_cast<std::uint64_t>(base);
    }

private:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::map<std::string, std::shared_ptr<ConfigFile>> m_pathToFile;
    std::map<std::shared_ptr<ConfigFile>, std::map<ConfigDataBase*, std::string>> m_fileToModules;
    std::string m_rootDir;
    std::shared_ptr<ConfigFile> m_defaultConfigFile;
};

} // namespace config
} // namespace afl
