/**
 * @file   ModuleManager.h
 * @brief  模块注册、实例化与变更通知
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/framework/EventDispatcher.h"
#include "afl/base/NonCopy.h"

#include <map>
#include <memory>
#include <mutex>

namespace afl
{
namespace fw
{
class Module;

/**
 * @brief 全局模块注册表与按序遍历
 */
class ModuleManager final : public afl::base::NonCopy
{
    using ModulePtr = std::shared_ptr<Module>;
    using ModuleWeakPtr = std::weak_ptr<Module>;
    using ModuleGenerator = std::function<ModulePtr()>;
    using ModuleForeachCallBack = std::function<void(ModulePtr)>;

    struct ModuleInfo
    {
        int serialNum;
        std::once_flag once;
        ModulePtr instance;
        ModuleGenerator generator;
    };

public:
    enum ModuleChange
    {
        McRegister,
        McUnRegister,
    };

    using ModuleMonitorDispatcher = afl::fw::EventDispatcher<std::string, void(ModuleChange)>;
    using ModuleMonitorCallback = ModuleMonitorDispatcher::Callback;

    /** @brief 模块变更监听器句柄 */
    using Handle = ModuleMonitorDispatcher::Handle;

    ModuleManager() = default;

    /**
     * @brief 注册模块工厂
     * @param key 模块唯一键
     * @param serial 遍历顺序（越小越靠前）
     * @param generator 实例化工厂，不可为空
     * @return 是否注册成功
     */
    bool registerModule(const std::string& key, int serial, const ModuleGenerator& generator);

    /**
     * @brief 注销模块
     * @param key 模块键
     */
    void unregisterModule(const std::string& key);

    /**
     * @brief 按类型注册模块（可选自定义工厂）
     * @tparam D 模块派生类型
     * @param key 模块键
     * @param serial 遍历顺序
     * @param generator 自定义工厂；为空则默认 make_shared<D>
     */
    template <typename D>
    void registerModule(const std::string& key, int serial = 100,
                        const ModuleGenerator& generator = nullptr)
    {
        if (generator)
        {
            registerModule(key, serial, generator);
        }
        else
        {
            registerModule(key, serial, []() { return std::make_shared<D>(); });
        }
    }

    /**
     * @brief 监听任意模块注册/注销
     * @param cb 回调
     * @return 监听器句柄
     */
    Handle addModuleChangeListener(const ModuleMonitorCallback& cb);

    /**
     * @brief 监听指定键的模块变更
     * @param kw 模块键
     * @param cb 回调
     * @return 监听器句柄
     */
    Handle addModuleChangeListener(const std::string& kw, const ModuleMonitorCallback& cb);

    /**
     * @brief 获取模块实例（懒创建）
     * @tparam I 期望的模块接口类型
     * @param key 模块键
     * @return 实例指针，未注册时为空
     */
    template <typename I = Module>
    std::shared_ptr<I> getModuleInstance(const std::string& key)
    {
        return std::static_pointer_cast<I>(findModuleInstance(key));
    }

    /** @brief 按 serial 升序遍历全部模块实例 */
    void forEach(const ModuleForeachCallBack& cb);

    /** @brief 按 serial 降序遍历全部模块实例 */
    void forEachReverse(const ModuleForeachCallBack& cb);

    /** @brief 已注册模块数量 */
    size_t count();

    /** @brief 键是否已注册 */
    bool isExist(const std::string& key);

    /** @brief 清空注册表（不销毁已创建实例） */
    void clear();

private:
    ModulePtr findModuleInstance(const std::string& key);
    ModulePtr generate(const std::string& key, ModuleInfo& info);

private:
    std::mutex m_storageLock;
    std::map<std::string, ModuleInfo> m_moduleStorage;
    ModuleMonitorDispatcher m_dispatcher;
};

} // namespace fw
} // namespace afl
