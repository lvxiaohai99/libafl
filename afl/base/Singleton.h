/**
 * @file   Singleton.h
 * @brief  线程安全的单例模板基类（合并原 base/singleton.h 与 base/design_pattern/singleton.h 两套实现）。
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include "afl/base/Common.h"

#include <array>
#include <memory>
#include <mutex>

namespace afl
{
namespace base
{
/** 声明单例友元，使 Singleton 基类可访问派生类私有构造函数 */
#define DECLARE_SINGLETON_CLASS(type) friend class afl::base::Singleton<type>

/**
 * @brief 线程安全单例基类
 *
 * - std::call_once 保证并发首次创建的唯一性；
 * - 内部 Proxy 对象在进程退出时自动释放实例；
 * - 派生类构造/析构函数建议声明为 private 并配合 DECLARE_SINGLETON_CLASS。
 */
template <typename T>
class Singleton
{
public:
    /** @brief 获取单例指针（不存在则创建） */
    static T* getInstancePtr()
    {
        T* p = m_proxy.m_instance;
        if (p == NULL)
        {
            std::lock_guard<std::mutex> guard(m_proxy.m_mutex);
            p = m_proxy.m_instance;
            if (p == NULL)
            {
                p = new T;
                m_proxy.m_instance = p;
            }
        }
        return p;
    }

    /** @brief 获取单例引用（不存在则创建） */
    static T& getInstance() { return *getInstancePtr(); }

    /** @brief 兼容旧接口：获取单例引用 */
    static T& getInstanceRef() { return getInstance(); }

    /** @brief 强制创建单例并返回指针（已存在则返回现有实例） */
    static T* createInstance() { return getInstancePtr(); }

    /** @brief 手动销毁单例，下次获取时会重建（进程退出时也会自动释放） */
    static void deleteInstance()
    {
        std::lock_guard<std::mutex> guard(m_proxy.m_mutex);
        delete m_proxy.m_instance;
        m_proxy.m_instance = NULL;
    }

protected:
    Singleton() {}
    ~Singleton() {}

private:
    /** 实例持有者，进程退出时自动释放 */
    struct Proxy
    {
        Proxy() : m_instance(NULL) {}

        ~Proxy()
        {
            if (m_instance)
            {
                delete m_instance;
                m_instance = NULL;
            }
        }

        T* m_instance;
        std::mutex m_mutex;
    };

    static Proxy m_proxy;

private:
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);
};

template <typename T>
typename Singleton<T>::Proxy Singleton<T>::m_proxy;

/**
 * @brief 多实例槽单例（原 design_pattern/singleton.h）
 *
 * 通过模板参数 N 指定实例槽位数，Instance<idx>() 获取指定槽的共享实例。
 * @note 当前库内暂无使用方，保留作为工具能力。
 */
template <typename T, size_t N = 1>
class MultiSingleton
{
public:
    /** @brief 获取第 0 槽实例 */
    template <typename... Args>
    static std::shared_ptr<T> instance(Args&&... args)
    {
        return instanceAt<0>(std::forward<Args>(args)...);
    }

    /** @brief 获取第 NN 槽实例（0 <= NN < N） */
    template <size_t NN, typename... Args>
    static std::shared_ptr<T> instanceAt(Args&&... args)
    {
        static_assert(NN < N, "instance index out of range!");
        std::call_once(m_onceArray[NN],
                       [&]() { m_instanceArray[NN].reset(new T(std::forward<Args>(args)...)); });
        return m_instanceArray[NN];
    }

    using element_type = T;         ///< 元素类型
    static const size_t kSlots = N; ///< 实例槽位数

private:
    MultiSingleton();
    ~MultiSingleton();
    MultiSingleton(const MultiSingleton&);
    MultiSingleton& operator=(const MultiSingleton&);

private:
    static std::array<std::shared_ptr<T>, N> m_instanceArray; ///< 实例数组
    static std::array<std::once_flag, N> m_onceArray;         ///< 每槽 once 标志
};

template <typename T, size_t N>
std::array<std::shared_ptr<T>, N> MultiSingleton<T, N>::m_instanceArray;

template <typename T, size_t N>
std::array<std::once_flag, N> MultiSingleton<T, N>::m_onceArray;

} // namespace base
} // namespace afl
