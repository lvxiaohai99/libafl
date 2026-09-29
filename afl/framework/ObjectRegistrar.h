/**
 * @file   ObjectRegistrar.h
 * @brief  按键注册/创建派生对象的工厂（可单例）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <map>
#include <mutex>

namespace afl
{
namespace fw
{
/**
 * @brief 对象注册表模板
 * @tparam B 基类类型
 * @tparam K 注册键类型，默认 std::string
 * @tparam multiKey 预留参数（兼容旧接口）
 *
 * 通过 RegisterHelper 在静态初始化阶段注册派生类生成器，
 * 再按键 getInstance 创建（或复用唯一实例）。
 */
template <typename B, typename K = std::string, int multiKey = 0>
class ObjectRegistrar
{
    using ObjectGenerator = std::function<std::shared_ptr<B>()>;

    struct DerivedObjectInfo
    {
        std::once_flag* once;
        std::shared_ptr<B> backup;
        ObjectGenerator generator;
    };

public:
    /**
     * @brief 静态注册辅助：构造时完成 registerObject
     * @tparam D 派生类
     */
    template <typename D>
    struct RegisterHelper
    {
        /**
         * @param key       注册键
         * @param unique    true 时同键只创建一次（单例）
         * @param generator 可选自定义工厂；空则默认 make_shared&lt;D&gt;
         */
        RegisterHelper(const K& key, bool unique, ObjectGenerator generator = nullptr)
        {
            ObjectRegistrar::registerObject<D>(key, unique, generator);
        }
    };

    /**
     * @brief 注册派生类型
     * @tparam D 派生类
     * @param key       注册键
     * @param unique    是否单例
     * @param generator 可选工厂
     */
    template <typename D>
    static void registerObject(const K& key, bool unique, ObjectGenerator generator = nullptr)
    {
        auto& storage = getObjectStorage();

        std::lock_guard<std::mutex> guard(objectStorageLock);
        storage[key] = {unique ? new std::once_flag() : nullptr, nullptr,
                        generator ? generator : []() { return std::make_shared<D>(); }};
    }

    /**
     * @brief 按键获取实例
     * @param key 注册键
     * @return 共享指针；未注册返回 nullptr
     */
    static std::shared_ptr<B> getInstance(const K& key)
    {
        auto& storage = getObjectStorage();

        auto iter = storage.find(key);
        if (iter == storage.end())
        {
            return nullptr;
        }

        return generate(iter->second);
    }

private:
    static std::map<K, DerivedObjectInfo>& getObjectStorage()
    {
        static std::map<K, DerivedObjectInfo> store;
        return store;
    }

    static std::shared_ptr<B> generate(DerivedObjectInfo& info)
    {
        if (info.once)
        {
            std::call_once(*info.once, [&info]() { info.backup = info.generator(); });
            return info.backup;
        }
        return info.generator();
    }

    static std::mutex objectStorageLock;
};

template <typename B, typename K, int multiKey>
std::mutex ObjectRegistrar<B, K, multiKey>::objectStorageLock;

}  // namespace fw
}  // namespace afl
