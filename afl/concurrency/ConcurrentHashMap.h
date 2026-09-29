/**
 * @file   ConcurrentHashMap.h
 * @brief  分段锁并发哈希表（简化实现，C++11）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/concurrency/Mutex.h"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <utility>
#include <vector>

namespace afl
{
namespace concurrency
{
/**
 * @brief 分桶 + 每桶独立互斥的并发 map
 * @tparam Key      键
 * @tparam Value    值
 * @tparam Hash     哈希函数
 * @tparam Equality 相等比较
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equality = std::equal_to<Key>>
class ConcurrentHashMap
{
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Hash hash_type;
    typedef Equality equality_type;
    typedef afl::concurrency::Mutex mutex_type;
    typedef afl::concurrency::LockGuard<mutex_type> lock_guard;

    /**
     * @brief 构造
     * @param numBuckets 桶数量（建议素数）
     * @param hasher     哈希器
     * @param equaler    相等比较器
     */
    explicit ConcurrentHashMap(size_t numBuckets = 19, const Hash& hasher = Hash(),
                               const Equality& equaler = Equality())
        : m_hasher(hasher), m_equaler(equaler), m_buckets(numBuckets)
    {
        for (size_t i = 0; i < numBuckets; ++i)
        {
            m_buckets[i] = new Bucket(equaler);
        }
    }

    ~ConcurrentHashMap()
    {
        for (size_t i = 0; i < m_buckets.size(); ++i)
        {
            delete m_buckets[i];
            m_buckets[i] = nullptr;
        }
    }

    ConcurrentHashMap(const ConcurrentHashMap&) = delete;
    ConcurrentHashMap& operator=(const ConcurrentHashMap&) = delete;

    /** @brief 是否包含键 @param key 键 @return 是否存在 */
    bool contain(const Key& key) const { return bucketFor(key).contain(key); }

    /**
     * @brief 取值
     * @param key 键
     * @param defaultValue 未找到时返回值
     * @return 值或默认值
     */
    Value get(const Key& key, const Value& defaultValue = Value()) const
    {
        return bucketFor(key).get(key, defaultValue);
    }

    /**
     * @brief 插入或覆盖
     * @param key   键
     * @param value 值
     */
    void put(const Key& key, const Value& value) { bucketFor(key).put(key, value); }

    /**
     * @brief 仅当键不存在时插入
     * @param key   键
     * @param value 值
     * @return 若已存在返回旧值，否则返回插入值
     */
    Value putIfAbsent(const Key& key, const Value& value)
    {
        return bucketFor(key).putIfAbsent(key, value);
    }

    /**
     * @brief 按键删除
     * @param key 键
     * @return 被删值；不存在返回 Value()
     */
    Value remove(const Key& key) { return bucketFor(key).remove(key); }

    /**
     * @brief 仅当当前值等于 exceptValue 时删除
     * @param key         键
     * @param exceptValue 期望值
     * @return 是否删除成功
     */
    bool remove(const Key& key, const Value& exceptValue)
    {
        return bucketFor(key).remove(key, exceptValue);
    }

    /** @brief 元素个数（遍历各桶加锁统计） */
    size_t size() const
    {
        size_t total = 0;
        for (size_t i = 0; i < m_buckets.size(); ++i)
        {
            total += m_buckets[i]->size();
        }
        return total;
    }

    /**
     * @brief 导出为普通 map（全局一致性快照，代价较高）
     * @return std::map 副本
     */
    std::map<Key, Value> toMap() const
    {
        std::map<Key, Value> result;
        for (size_t i = 0; i < m_buckets.size(); ++i)
        {
            m_buckets[i]->copyInto(result);
        }
        return result;
    }

private:
    class Bucket
    {
    public:
        typedef std::pair<Key, Value> Entry;
        typedef std::list<Entry> EntryList;

        explicit Bucket(const Equality& equaler) : m_equaler(equaler) {}

        bool contain(const Key& key) const
        {
            lock_guard lock(m_mutex);
            return findUnlocked(key) != m_data.end();
        }

        Value get(const Key& key, const Value& defaultValue) const
        {
            lock_guard lock(m_mutex);
            typename EntryList::const_iterator it = findUnlocked(key);
            return (it == m_data.end()) ? defaultValue : it->second;
        }

        void put(const Key& key, const Value& value)
        {
            lock_guard lock(m_mutex);
            typename EntryList::iterator it = findUnlocked(key);
            if (it == m_data.end())
            {
                m_data.push_back(Entry(key, value));
            }
            else
            {
                it->second = value;
            }
        }

        Value putIfAbsent(const Key& key, const Value& value)
        {
            lock_guard lock(m_mutex);
            typename EntryList::iterator it = findUnlocked(key);
            if (it == m_data.end())
            {
                m_data.push_back(Entry(key, value));
                return value;
            }
            return it->second;
        }

        Value remove(const Key& key)
        {
            Value old = Value();
            lock_guard lock(m_mutex);
            typename EntryList::iterator it = findUnlocked(key);
            if (it != m_data.end())
            {
                old = it->second;
                m_data.erase(it);
            }
            return old;
        }

        bool remove(const Key& key, const Value& exceptValue)
        {
            lock_guard lock(m_mutex);
            typename EntryList::iterator it = findUnlocked(key);
            if (it != m_data.end() && it->second == exceptValue)
            {
                m_data.erase(it);
                return true;
            }
            return false;
        }

        size_t size() const
        {
            lock_guard lock(m_mutex);
            return m_data.size();
        }

        void copyInto(std::map<Key, Value>& out) const
        {
            lock_guard lock(m_mutex);
            for (typename EntryList::const_iterator it = m_data.begin(); it != m_data.end(); ++it)
            {
                out.insert(*it);
            }
        }

    private:
        typename EntryList::iterator findUnlocked(const Key& key)
        {
            return std::find_if(m_data.begin(), m_data.end(),
                                [&](const Entry& item) { return m_equaler(item.first, key); });
        }

        typename EntryList::const_iterator findUnlocked(const Key& key) const
        {
            return std::find_if(m_data.begin(), m_data.end(),
                                [&](const Entry& item) { return m_equaler(item.first, key); });
        }

        Equality m_equaler;
        EntryList m_data;
        mutable mutex_type m_mutex;
    };

    Bucket& bucketFor(const Key& key) const
    {
        const size_t index = m_hasher(key) % m_buckets.size();
        return *m_buckets[index];
    }

    Hash m_hasher;
    Equality m_equaler;
    std::vector<Bucket*> m_buckets;
};

} // namespace concurrency
} // namespace afl
