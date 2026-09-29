/**
 * @file   Signal.h
 * @brief  线程安全的信号/槽（观察者模式），语义对齐 Qt signals/slots 与 nod
 * @author libafl
 * @date   2026-09
 *
 * 设计参考 Nano Signal Slot（nod）：槽以 std::function 保存，连接返回可断开的 Connection；
 * 发射时拷贝槽表，允许槽内再 connect/disconnect。默认多线程策略带互斥。
 *
 * 典型用法（对照 MetaEvent::m_Signal）：
 * @code
 *   afl::base::Signal<void(int, const char*)> sig;
 *   auto conn = sig.connect([](int a, const char* s) { ... });
 *   sig(42, "hello");
 *   conn.disconnect();
 * @endcode
 */
#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <thread>
#include <utility>
#include <cstddef>

namespace afl
{
namespace base
{
namespace signalDetail
{
/** @brief 类型擦除的断开接口，供 Connection 持有 */
struct Disconnector
{
    virtual ~Disconnector() {}
    virtual void operator()(std::size_t index) const = 0;
};

/** @brief 不释放的 deleter，Disconnector 嵌入 Signal 对象内时使用 */
inline void noDelete(Disconnector*) {}

}  // namespace signalDetail

template <typename Policy, typename Signature>
class SignalType;

/**
 * @brief 连接句柄：可手动 disconnect；不可拷贝，可移动
 */
class Connection
{
public:
    Connection() : m_index(0) {}

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other)
        : m_weakDisconnector(std::move(other.m_weakDisconnector)), m_index(other.m_index)
    {
    }

    Connection& operator=(Connection&& other)
    {
        m_weakDisconnector = std::move(other.m_weakDisconnector);
        m_index = other.m_index;
        return *this;
    }

    /** @brief 是否仍连接到某个 Signal */
    bool connected() const { return !m_weakDisconnector.expired(); }

    /** @brief 断开槽；已断开或 Signal 已销毁则为空操作 */
    void disconnect()
    {
        auto ptr = m_weakDisconnector.lock();
        if (ptr)
        {
            (*ptr)(m_index);
        }
        m_weakDisconnector.reset();
    }

private:
    template <typename P, typename T>
    friend class SignalType;

    Connection(const std::shared_ptr<signalDetail::Disconnector>& sharedDisconnector,
               std::size_t index)
        : m_weakDisconnector(sharedDisconnector), m_index(index)
    {
    }

    std::weak_ptr<signalDetail::Disconnector> m_weakDisconnector;
    std::size_t m_index;
};

/**
 * @brief RAII 连接：析构时自动 disconnect
 */
class ScopedConnection
{
public:
    ScopedConnection() = default;
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) : m_connection(std::move(other.m_connection)) {}

    ScopedConnection& operator=(ScopedConnection&& other)
    {
        reset(std::move(other.m_connection));
        return *this;
    }

    /** @brief 允许 ScopedConnection c = sig.connect(...) 写法 */
    ScopedConnection(Connection&& c) : m_connection(std::move(c)) {}

    ~ScopedConnection() { disconnect(); }

    ScopedConnection& operator=(Connection&& c)
    {
        reset(std::move(c));
        return *this;
    }

    void reset(Connection&& c = Connection())
    {
        disconnect();
        m_connection = std::move(c);
    }

    Connection release()
    {
        Connection c = std::move(m_connection);
        m_connection = Connection();
        return c;
    }

    bool connected() const { return m_connection.connected(); }

    void disconnect() { m_connection.disconnect(); }

private:
    Connection m_connection;
};

/** @brief 多线程策略：互斥保护槽表 */
struct MultiThreadPolicy
{
    using MutexType = std::mutex;
    using MutexLockType = std::lock_guard<MutexType>;
    static void yieldThread() { std::this_thread::yield(); }
};

/** @brief 单线程策略：无锁，仅用于确认无跨线程访问的场景 */
struct SingleThreadPolicy
{
    struct MutexType
    {
    };
    struct MutexLockType
    {
        explicit MutexLockType(const MutexType&) {}
    };
    static void yieldThread() {}
};

/**
 * @brief 信号模板（按线程策略参数化）
 * @tparam Policy  MultiThreadPolicy / SingleThreadPolicy
 * @tparam R       槽返回类型（常用 void）
 * @tparam Args    槽参数类型
 */
template <typename Policy, typename R, typename... Args>
class SignalType<Policy, R(Args...)>
{
public:
    using SlotType = std::function<R(Args...)>;
    using SizeType = typename std::vector<SlotType>::size_type;

    SignalType() : m_slotCount(0) {}

    SignalType(const SignalType&) = delete;
    SignalType& operator=(const SignalType&) = delete;

    ~SignalType() { invalidateDisconnector(); }

    /**
     * @brief 连接槽
     * @tparam T 可调用对象，签名须匹配 R(Args...)
     * @param slot 槽
     * @return 可用于断开的 Connection
     */
    template <typename T>
    Connection connect(T&& slot)
    {
        MutexLockType lock(m_mutex);
        m_slots.push_back(std::forward<T>(slot));
        const std::size_t index = m_slots.size() - 1;
        if (!m_sharedDisconnector)
        {
            m_disconnector = DisconnectorImpl(this);
            m_sharedDisconnector.reset(&m_disconnector, signalDetail::noDelete);
        }
        ++m_slotCount;
        return Connection(m_sharedDisconnector, index);
    }

    /**
     * @brief 发射信号（按连接顺序调用槽）
     * @param args 传给各槽的参数
     */
    void operator()(const Args&... args) const
    {
        for (const auto& slot : copySlots())
        {
            if (slot)
            {
                slot(args...);
            }
        }
    }

    /** @brief 当前有效槽数量 */
    SizeType slotCount() const { return m_slotCount; }

    /** @brief 是否无槽 */
    bool empty() const { return slotCount() == 0; }

    /** @brief 断开全部槽（会使既有 Connection 无法再 disconnect） */
    void disconnectAllSlots()
    {
        MutexLockType lock(m_mutex);
        m_slots.clear();
        m_slotCount = 0;
        invalidateDisconnector();
    }

private:
    using MutexType = typename Policy::MutexType;
    using MutexLockType = typename Policy::MutexLockType;

    void invalidateDisconnector()
    {
        std::weak_ptr<signalDetail::Disconnector> weak(m_sharedDisconnector);
        m_sharedDisconnector.reset();
        while (weak.lock() != nullptr)
        {
            Policy::yieldThread();
        }
    }

    std::vector<SlotType> copySlots() const
    {
        MutexLockType lock(m_mutex);
        return m_slots;
    }

    void disconnect(std::size_t index)
    {
        MutexLockType lock(m_mutex);
        if (index < m_slots.size() && m_slots[index])
        {
            --m_slotCount;
            m_slots[index] = SlotType();
        }
    }

    struct DisconnectorImpl : signalDetail::Disconnector
    {
        DisconnectorImpl() : m_ptr(nullptr) {}
        explicit DisconnectorImpl(SignalType* ptr) : m_ptr(ptr) {}

        void operator()(std::size_t index) const override
        {
            if (m_ptr)
            {
                m_ptr->disconnect(index);
            }
        }

        SignalType* m_ptr;
    };

    mutable MutexType m_mutex;
    std::vector<SlotType> m_slots;
    SizeType m_slotCount;
    DisconnectorImpl m_disconnector;
    std::shared_ptr<signalDetail::Disconnector> m_sharedDisconnector;
};

/**
 * @brief 默认线程安全信号（推荐）
 * @tparam Signature 函数签名，如 void(int)、void(const Rx&, int)
 *
 * 对应 nod::signal&lt;Signature&gt; / MetaEvent::m_Signal。
 */
template <typename Signature>
using Signal = SignalType<MultiThreadPolicy, Signature>;

/**
 * @brief 无锁信号（仅单线程、追求极致性能时使用）
 */
template <typename Signature>
using UnsafeSignal = SignalType<SingleThreadPolicy, Signature>;

}  // namespace base
}  // namespace afl
