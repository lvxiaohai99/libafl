/**
 * @file   ConfigManager.cpp
 * @brief  配置管理器实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/config/ConfigManager.h"
#include "afl/log/Log.h"

#include <cstdio>

namespace afl
{
namespace config
{
ConfigManager::ConfigManager(std::string root)
{
    if (root.empty())
    {
        root = "./";
    }
    if (root.back() != '/')
    {
        root.push_back('/');
    }
    m_rootDir = std::move(root);

    const std::string file = m_rootDir + "default.cfg";
    if (!afl::file::FileUtil::createRecursionDir(afl::file::FileUtil::dirName(file.c_str()).c_str()))
    {
        LOG_WARN("ConfigManager: create root dir failed root=%s", m_rootDir.c_str());
    }

    m_defaultConfigFile.reset(new ConfigFile(file));
    m_pathToFile[m_defaultConfigFile->fileName()] = m_defaultConfigFile;
    LOG_INFO("ConfigManager: root=%s default=%s", m_rootDir.c_str(), file.c_str());
}

ConfigManager::~ConfigManager()
{
    m_pathToFile.clear();
    m_fileToModules.clear();
}

std::shared_ptr<ConfigFile> ConfigManager::getConfigFile(std::uint64_t handle)
{
    ConfigDataBase* base = reinterpret_cast<ConfigDataBase*>(handle);
    const std::string& file = base->getFileName();
    auto it = m_pathToFile.find(file);
    if (it == m_pathToFile.end())
    {
        LOG_WARN("ConfigManager: unknown file %s", file.c_str());
        return nullptr;
    }
    return it->second;
}

void ConfigManager::unregisterConfig(std::uint64_t handle)
{
    ConfigDataBase* base = reinterpret_cast<ConfigDataBase*>(handle);
    const std::string& file = base->getFileName();
    auto fit = m_pathToFile.find(file);
    if (fit == m_pathToFile.end())
    {
        LOG_WARN("ConfigManager::unregisterConfig unknown file %s", file.c_str());
        return;
    }

    auto mit = m_fileToModules.find(fit->second);
    if (mit == m_fileToModules.end())
    {
        return;
    }

    auto dit = mit->second.find(base);
    if (dit == mit->second.end())
    {
        return;
    }
    mit->second.erase(dit);
}

void ConfigManager::saveConfig()
{
    for (auto& modules : m_fileToModules)
    {
        for (auto& entry : modules.second)
        {
            entry.first->save(entry.second, modules.first);
        }
    }
    for (auto& kv : m_pathToFile)
    {
        kv.second->save();
    }
}

void ConfigManager::reloadConfig()
{
    for (auto& kv : m_pathToFile)
    {
        reloadConfig(kv.first);
    }
}

void ConfigManager::reloadConfig(std::string path)
{
    if (m_pathToFile.find(path) == m_pathToFile.end())
    {
        LOG_ERROR("ConfigManager::reloadConfig unknown path=%s", path.c_str());
        throw std::logic_error("ConfigManager: no configure file called " + path);
    }

    LOG_INFO("ConfigManager::reloadConfig path=%s", path.c_str());
    m_pathToFile[path]->reload();

    auto mit = m_fileToModules.find(m_pathToFile[path]);
    if (mit == m_fileToModules.end())
    {
        return;
    }

    for (auto& entry : mit->second)
    {
        entry.first->reload(entry.second, m_pathToFile[path]);
    }
    m_pathToFile[path]->save();
}

} // namespace config
} // namespace afl
