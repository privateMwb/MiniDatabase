#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "Node.h"

template<typename K, typename V, typename Hash = std::hash<K>>
class LRUCache {
private:
    // Capacity
    std::size_t capacity_;

    // Sentinel Nodes
    Node<K, V>* head;
    Node<K, V>* tail;

    // Lookup Table
    std::unordered_map<K, Node<K, V>*, Hash> cache;

    // Cache Statistics
    std::size_t hitCounter;
    std::size_t missCounter;

    // List Operations
    void insertFront(Node<K, V>* node);
    void removeNode(Node<K, V>* node);
    void moveToFront(Node<K, V>* node);
    Node<K, V>* popBack();

    // Lookup Helpers
    [[nodiscard]] Node<K, V>* findNode(const K& key);
    [[nodiscard]] const Node<K, V>* findNode(const K& key) const;

    // Resource Management
    void reserve(std::size_t count);
    void release() noexcept;

public:
    // Lifecycle
    explicit LRUCache(std::size_t capacity);
    ~LRUCache();

    LRUCache(const LRUCache&)            = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    LRUCache(LRUCache&& other) noexcept;
    LRUCache& operator=(LRUCache&& other) noexcept;

    // Cache Modifiers
    void put(const K& key, const V& value);
    void put(const K& key, V&& value);
    void put(K&& key, V&& value);
    [[nodiscard]] bool erase(const K& key);
    void clear();

    // Cache Lookup
    [[nodiscard]] V* get(const K& key);
    [[nodiscard]] const V* peek(const K& key) const;
    [[nodiscard]] bool contains(const K& key) const;

    // Capacity Management
    void resize(std::size_t newCapacity);
    
    // Element Access
    [[nodiscard]] std::vector<K> keys() const;
    [[nodiscard]] const K* mostRecentKey() const;
    [[nodiscard]] const K* leastRecentKey() const;

    // Cache Statistics
    [[nodiscard]] std::size_t hitCount() const noexcept;
    [[nodiscard]] std::size_t missCount() const noexcept;
    [[nodiscard]] double hitRate() const noexcept;
    void resetStats() noexcept;

    // Capacity
    [[nodiscard]] std::size_t capacity() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    // State
    [[nodiscard]] bool empty() const noexcept;
};

#include "LRUCache.tpp"
