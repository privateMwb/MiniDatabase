#pragma once

#include <cstddef>
#include <functional>
#include <iterator>

#include "Iterator.h"
#include "Node.h"

template<typename K,
         typename V,
         typename Hash = std::hash<K>
         >
class HashMap {
public:

	// Iterators
	using iterator = Iterator<K, V>;
	using const_iterator = Iterator<K, V, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    
    // Configuration
    static constexpr double MAX_LOAD_FACTOR = 0.75;
    
    // Core Storage
    Node<K, V>** buckets;
    
	std::size_t bucketCount;
	std::size_t elementCount;
    
	Hash hasher;

	// Private Helper
	[[nodiscard]] std::size_t bucketIndex(const K& key) const;
	
	[[nodiscard]] Node<K, V>* findNode(const K& key);
	[[nodiscard]] const Node<K, V>* findNode(const K& key) const;
    
    void rehash(std::size_t newBucketCount);
    
    void release() noexcept;
    
public:

	// Lifecycle
	explicit HashMap(std::size_t bucketCount = 16);
	~HashMap();

	HashMap(const HashMap& other);
	HashMap& operator=(const HashMap& other);

	HashMap(HashMap&& other) noexcept;
	HashMap& operator=(HashMap&& other) noexcept;

	// Access
	[[nodiscard]] V& operator[](const K& key);

	[[nodiscard]] V& at(const K& key);
	[[nodiscard]] const V& at(const K& key) const;

	// Modifiers
	void insert(const K& key, const V& value);
	[[nodiscard]] bool update(const K& key, const V& value);
	[[nodiscard]] bool erase(const K& key);
	void clear();
	
	// Lookup
	[[nodiscard]] iterator find(const K& key);
	[[nodiscard]] const_iterator find(const K& key) const;
	
	[[nodiscard]] bool contains(const K& key) const noexcept;

	// Capacity
	[[nodiscard]] std::size_t capacity() const noexcept;
	[[nodiscard]] std::size_t size() const noexcept;

	// State
	[[nodiscard]] bool empty() const noexcept;

	// Iterator
	[[nodiscard]] iterator begin() noexcept;
	[[nodiscard]] iterator end() noexcept;
	[[nodiscard]] const_iterator begin() const noexcept;
	[[nodiscard]] const_iterator end() const noexcept;
	[[nodiscard]] const_iterator cbegin() const noexcept;
	[[nodiscard]] const_iterator cend() const noexcept;

	[[nodiscard]] reverse_iterator rbegin() noexcept;
	[[nodiscard]] reverse_iterator rend() noexcept;
	[[nodiscard]] const_reverse_iterator rbegin() const noexcept;
	[[nodiscard]] const_reverse_iterator rend() const noexcept;
	[[nodiscard]] const_reverse_iterator crbegin() const noexcept;
	[[nodiscard]] const_reverse_iterator crend() const noexcept;

};

#include "HashMap.tpp"


