/**
 * @file   EventDispatcher.h
 * @brief  按事件键分发的回调链表与 EventDispatcher
 * @author libafl
 * @date   2026-09
 */
#pragma once
#include <unistd.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <map>
#include <functional>

namespace afl
{
namespace fw
{
namespace internal
{
class NodeBase
{
};

/** @brief 无操作互斥量，供 CallableList 在无锁场景下占位 */
struct NullMutex
{
    void lock() {}
    void unlock() {}
};

} // namespace internal

/**
 * @brief 可共享的回调链表（支持增删与遍历）
 * @details 须通过 shared_ptr 管理生命周期，例如：\n
 *      1. std::make_shared<CallableList<>>()\n
 *      2. std::shared_ptr<CallableList<>>(new CallableList<>)
 */
template <typename F, typename N = internal::NodeBase, typename M = internal::NullMutex>
class CallableList;

template <typename N, typename M, typename R, typename... Args>
class CallableList<R(Args...), N, M> final
    : public std::enable_shared_from_this<CallableList<R(Args...), N, M>>
{
public:
    using Callback = std::function<R(Args...)>;

private:
    using Counter = uint8_t;
    using Mutex = M;
    using ThisType = CallableList<R(Args...), N, M>;

    struct Node;
    using NodePtr = std::shared_ptr<Node>;
    class Node : public N
    {
    public:
        Node(const Callback& callback, const Counter counter) : callback(callback), counter(counter)
        {
        }

    private:
        void setOwner(std::weak_ptr<ThisType> owner) { m_owner = owner; }

    private:
        NodePtr prev, next;
        Callback callback;
        Counter counter;
        std::weak_ptr<ThisType> m_owner;

        friend class CallableList<R(Args...), N, M>;
    };

public:
    class Handle : public std::weak_ptr<Node>
    {
        using std::weak_ptr<Node>::weak_ptr;

    public:
        operator bool() const noexcept { return !this->expired(); }

        std::shared_ptr<N> content() { return this->lock(); }

        bool enable()
        {
            auto node = this->lock();
            return node ? (node->counter = enabledCounter, true) : false;
        }

        bool disable()
        {
            auto node = this->lock();
            return node ? (node->counter = disabledCounter, true) : false;
        }

        /**
         * @brief 从链表中移除本节点
         * @note 移除后仍可能再被调用一次，请注意竞态
         */
        void remove()
        {
            auto node = this->lock();
            if (node)
            {
                auto owner = node->m_owner.lock();
                if (owner)
                {
                    owner->doRemove(node);
                }
            }
        }

        /**
         * @brief 移除并等待节点引用全部释放
         * @note 会阻塞直至 expired；移除后不应再触发回调
         */
        void removeExactly()
        {
            remove();
            while (!(this->expired()))
            {
                usleep(10);
            }
        }
    };

private:
    enum : Counter
    {
        disabledCounter = 0,
        enabledCounter = 1
    };

public:
    CallableList() noexcept : head(), tail(), mutex(), currentCounter(0) {}

    ~CallableList()
    {
        // 析构时不加锁，避免异常路径下二次抛错
        NodePtr node = head;
        head.reset();
        while (node)
        {
            NodePtr next = node->next;
            node->prev.reset();
            node->next.reset();
            node = next;
        }
        node.reset();
    }

    bool empty() const
    {
        // 为性能不加锁；其它线程可能正在写 head，!head 仍可作为近似判断
        // 返回后链表是否仍为空无保证（非原子快照）
        //std::lock_guard<Mutex> lockGuard(mutex);
        return !head;
    }

    operator bool() const { return !empty(); }

    Handle append(const Callback& callback)
    {
        NodePtr node(doAllocateNode(callback));

        {
            std::lock_guard<Mutex> lockGuard(mutex);
            if (head)
            {
                node->prev = tail;
                tail->next = node;
                tail = node;
            }
            else
            {
                head = node;
                tail = node;
            }
        }

        return Handle(node);
    }

    Handle prepend(const Callback& callback)
    {
        NodePtr node(doAllocateNode(callback));

        {
            std::lock_guard<Mutex> lockGuard(mutex);
            if (head)
            {
                node->next = head;
                head->prev = node;
                head = node;
            }
            else
            {
                head = node;
                tail = node;
            }
        }

        return Handle(node);
    }

    Handle insert(const Callback& callback, const Handle& before)
    {
        NodePtr beforeNode = before.lock();
        if (beforeNode)
        {
            NodePtr node(doAllocateNode(callback));
            {
                std::lock_guard<Mutex> lockGuard(mutex);
                doInsert(node, beforeNode);
            }
            return Handle(node);
        }

        return append(callback);
    }

    template <typename Func>
    void forEach(Func&& func) const
    {
        doForEachIf([&func, this](NodePtr& node) -> bool {
            func(Handle(node), node->callback);
            return true;
        });
    }

    template <typename Func>
    bool forEachIf(Func&& func) const
    {
        return doForEachIf(
            [&func, this](NodePtr& node) -> bool { return func(Handle(node), node->callback); });
    }

    void invoke(Args... args) const
    {
        forEachIf([&args...](Handle, Callback& callback) -> bool {
            callback(args...);
            return true;
        });
    }

private:
    CallableList(const CallableList& other) = delete;
    CallableList(CallableList&& other) = delete;
    CallableList& operator=(const CallableList& other) = delete;
    CallableList& operator=(CallableList&& other) = delete;

    template <typename F>
    bool doForEachIf(F&& f) const
    {
        NodePtr node;
        {
            std::lock_guard<Mutex> lockGuard(mutex);
            node = head;
        }

        const Counter counter = currentCounter.load(std::memory_order_acquire);
        while (node)
        {
            if (node->counter != disabledCounter && counter >= node->counter)
            {
                if (!f(node))
                {
                    return false;
                }
            }
            {
                std::lock_guard<Mutex> lockGuard(mutex);
                node = node->next;
            }
        }
        return true;
    }

    void doInsert(NodePtr& node, NodePtr& beforeNode)
    {
        node->prev = beforeNode->prev;
        node->next = beforeNode;
        if (beforeNode->prev)
        {
            beforeNode->prev->next = node;
        }
        beforeNode->prev = node;

        if (beforeNode == head)
        {
            head = node;
        }
    }

    NodePtr doAllocateNode(const Callback& callback)
    {
        auto retv = std::make_shared<Node>(callback, getNextCounter());
        retv->setOwner(std::enable_shared_from_this<ThisType>::shared_from_this());
        return retv;
    }

    /**
     * @brief 从链表中摘除节点（不修改 node 的 prev/next 指针）
     * @note 遍历中可能仍持有 node，故只改链表结构
     */
    void doFreeNode(NodePtr& node)
    {
        if (node->next)
        {
            node->next->prev = node->prev;
        }
        if (node->prev)
        {
            node->prev->next = node->next;
        }

        if (head == node)
        {
            head = node->next;
        }
        if (tail == node)
        {
            tail = node->prev;
        }

        node->counter = disabledCounter;
    }

    bool doRemove(NodePtr node)
    {
        if (node)
        {
            std::lock_guard<Mutex> lockGuard(mutex);
            doFreeNode(node);
            return true;
        }
        return false;
    }

    Counter getNextCounter()
    {
        Counter result = ++currentCounter;
        if (disabledCounter == result)
        {
            // 计数器溢出，重置各节点 counter
            {
                std::lock_guard<Mutex> lockGuard(mutex);
                NodePtr node = head;
                while (node)
                {
                    node->counter = enabledCounter;
                    node = node->next;
                }
            }
            result = ++currentCounter;
        }

        return result;
    }

private:
    NodePtr head, tail;
    mutable Mutex mutex;
    std::atomic<Counter> currentCounter;
};

template <typename E, typename F, typename N = internal::NodeBase, typename M = std::mutex>
class EventDispatcher;

/**
 * @brief 按事件键分发回调；支持全局监听与按键过滤
 */
template <typename E, typename N, typename M, typename R, typename... Args>
class EventDispatcher<E, R(Args...), N, M>
{
    using CallableListType = CallableList<R(E, Args...), N, M>;
    using CallableListPtr = std::shared_ptr<CallableListType>;
    using EventCallableListMap = std::map<E, CallableListPtr>;
    using Mutex = M;

public:
    using Event = E;
    using Callback = typename CallableListType::Callback;
    using Handle = typename CallableListType::Handle;

public:
    EventDispatcher() : m_eventCallableListMap(), m_listenerMutex()
    {
        m_listForNofilteringReceiver = std::make_shared<CallableListType>();
    }

    Handle append(const Callback& callback)
    {
        return m_listForNofilteringReceiver->append(callback);
    }

    Handle prepend(const Callback& callback)
    {
        return m_listForNofilteringReceiver->prepend(callback);
    }

    Handle insert(const Callback& callback, const Handle& before)
    {
        return m_listForNofilteringReceiver->insert(callback, before);
    }

    Handle append(const Event& event, const Callback& callback)
    {
        return doEnsureCallableList(event)->append(callback);
    }

    Handle prepend(const Event& event, const Callback& callback)
    {
        return doEnsureCallableList(event)->prepend(callback);
    }

    Handle insert(const Event& event, const Callback& callback, const Handle& before)
    {
        return doEnsureCallableList(event)->insert(callback, before);
    }

    template <typename Func>
    void forEach(const Event& event, Func&& func)
    {
        CallableListPtr callableList = doFindCallableList(event);
        if (callableList)
        {
            callableList->forEach(func);
        }
        m_listForNofilteringReceiver->forEach(std::forward<Func>(func));
    }

    template <typename Func>
    bool forEachIf(const Event& event, Func&& func)
    {
        CallableListPtr callableList = doFindCallableList(event);
        if (callableList)
        {
            return callableList->forEachIf(func);
        }

        m_listForNofilteringReceiver->forEachIf(std::forward<Func>(func));
        return true;
    }

    void invoke(const Event& event, Args... args)
    {
        CallableListPtr callableList = doFindCallableList(event);
        if (callableList)
        {
            callableList->invoke(event, args...);
        }

        m_listForNofilteringReceiver->invoke(event, args...);
    }

private:
    CallableListPtr doFindCallableList(const Event& e)
    {
        std::lock_guard<Mutex> lockGuard(m_listenerMutex);
        auto iter = m_eventCallableListMap.find(e);
        return iter != m_eventCallableListMap.end() ? iter->second : nullptr;
    }

    CallableListPtr doEnsureCallableList(const Event& e)
    {
        std::lock_guard<Mutex> lockGuard(m_listenerMutex);
        auto iter = m_eventCallableListMap.find(e);

        if (iter == m_eventCallableListMap.end())
        {
            auto cbList = std::make_shared<CallableListType>();
            m_eventCallableListMap[e] = cbList;
            return cbList;
        }

        return iter->second;
    }

private:
    EventDispatcher(const EventDispatcher& other) = delete;
    EventDispatcher(EventDispatcher&& other) = delete;
    EventDispatcher& operator=(const EventDispatcher& other) = delete;
    EventDispatcher& operator=(EventDispatcher&& other) = delete;

private:
    CallableListPtr m_listForNofilteringReceiver;
    EventCallableListMap m_eventCallableListMap;
    mutable Mutex m_listenerMutex;
};

} // namespace fw
} // namespace afl
