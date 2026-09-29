/**
 * @file   ModuleManager.cpp
 * @brief  模块管理器实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/framework/ModuleManager.h"
#include "afl/framework/Module.h"
#include "afl/log/Log.h"

#include <cstdlib>
#include <exception>

int pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)
    __attribute__((weak));

namespace afl
{
namespace fw
{
bool ModuleManager::registerModule(const std::string& key, int serial /*= 100*/,
                                   const ModuleGenerator& generator /*= nullptr*/)
{
    if (key.empty() || !generator)
    {
        LOG_ERROR("ModuleManager::registerModule invalid key/generator key=[%s]", key.c_str());
        return false;
    }

    {
        std::lock_guard<std::mutex> guard(m_storageLock);
        m_moduleStorage[key].generator = generator;
        m_moduleStorage[key].serialNum = serial;
    }

    LOG_INFO("ModuleManager register key=%s serial=%d", key.c_str(), serial);
    m_dispatcher.invoke(key, McRegister);
    return true;
}

void ModuleManager::unregisterModule(const std::string& key)
{
    {
        std::lock_guard<std::mutex> guard(m_storageLock);
        auto iter = m_moduleStorage.find(key);
        if (iter == m_moduleStorage.end())
        {
            LOG_WARN("ModuleManager::unregisterModule not found key=%s", key.c_str());
            return;
        }
        m_moduleStorage.erase(iter);
    }
    LOG_INFO("ModuleManager unregister key=%s", key.c_str());
    m_dispatcher.invoke(key, McUnRegister);
}

ModuleManager::Handle ModuleManager::addModuleChangeListener(const ModuleMonitorCallback& cb)
{
    return m_dispatcher.prepend(cb);
}

ModuleManager::Handle ModuleManager::addModuleChangeListener(const std::string& kw,
                                                             const ModuleMonitorCallback& cb)
{
    return m_dispatcher.prepend(kw, cb);
}

void ModuleManager::forEach(const ModuleForeachCallBack& cb)
{
    std::multimap<int, ModulePtr> modules;
    {
        std::lock_guard<std::mutex> guard(m_storageLock);
        for (auto& m : m_moduleStorage)
        {
            ModulePtr inst = generate(m.first, m.second);
            if (!inst)
            {
                LOG_ERROR("ModuleManager::forEach generate failed key=%s", m.first.c_str());
                continue;
            }
            modules.emplace(m.second.serialNum, inst);
        }
    }

    LOG_DEBUG("ModuleManager::forEach count=%zu", modules.size());
    for (auto iter = modules.begin(); iter != modules.end(); iter++)
    {
        cb(iter->second);
    }
}

void ModuleManager::forEachReverse(const ModuleForeachCallBack& cb)
{
    std::multimap<int, ModulePtr> modules;
    {
        std::lock_guard<std::mutex> guard(m_storageLock);
        for (auto& m : m_moduleStorage)
        {
            ModulePtr inst = generate(m.first, m.second);
            if (!inst)
            {
                LOG_ERROR("ModuleManager::forEachReverse generate failed key=%s", m.first.c_str());
                continue;
            }
            modules.emplace(m.second.serialNum, inst);
        }
    }

    LOG_DEBUG("ModuleManager::forEachReverse count=%zu", modules.size());
    for (auto iter = modules.rbegin(); iter != modules.rend(); iter++)
    {
        cb(iter->second);
    }
}

size_t ModuleManager::count()
{
    std::lock_guard<std::mutex> guard(m_storageLock);
    return m_moduleStorage.size();
}

bool ModuleManager::isExist(const std::string& key)
{
    std::lock_guard<std::mutex> guard(m_storageLock);
    return m_moduleStorage.find(key) != m_moduleStorage.end();
}

void ModuleManager::clear()
{
    std::lock_guard<std::mutex> guard(m_storageLock);
    LOG_INFO("ModuleManager::clear size=%zu", m_moduleStorage.size());
    m_moduleStorage.clear();
}

ModuleManager::ModulePtr ModuleManager::findModuleInstance(const std::string& key)
{
    std::lock_guard<std::mutex> guard(m_storageLock);
    auto iter = m_moduleStorage.find(key);
    if (iter == m_moduleStorage.end())
    {
        LOG_DEBUG("ModuleManager::getModuleInstance miss key=%s", key.c_str());
        return nullptr;
    }
    return generate(key, iter->second);
}

ModuleManager::ModulePtr ModuleManager::generate(const std::string& key, ModuleInfo& info)
{
    if (!info.generator)
    {
        LOG_ERROR("ModuleManager::generate null generator key=%s", key.c_str());
        return nullptr;
    }

    try
    {
        if (pthread_create)
        {
            std::call_once(info.once, [&]() {
                info.instance = info.generator();
                if (info.instance)
                {
                    info.instance->setName(key);
                    LOG_DEBUG("ModuleManager::generate created key=%s", key.c_str());
                }
                else
                {
                    LOG_ERROR("ModuleManager::generate factory returned null key=%s", key.c_str());
                }
            });
        }
        else
        {
            if (!info.instance)
            {
                info.instance = info.generator();
                if (info.instance)
                {
                    info.instance->setName(key);
                    LOG_DEBUG("ModuleManager::generate created key=%s", key.c_str());
                }
                else
                {
                    LOG_ERROR("ModuleManager::generate factory returned null key=%s", key.c_str());
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("ModuleManager::generate exception key=%s what=%s", key.c_str(), e.what());
        return nullptr;
    }
    catch (...)
    {
        LOG_ERROR("ModuleManager::generate unknown exception key=%s", key.c_str());
        return nullptr;
    }

    return info.instance;
}

} // namespace fw
} // namespace afl
