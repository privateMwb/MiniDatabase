#include <stdexcept>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

// List Operations
template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::insertFront(Node<K, V>* node) {
	node->next = head->next;
	node->prev = head;

	head->next->prev = node;
	head->next       = node;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::removeNode(Node<K, V>* node) {
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::moveToFront(Node<K, V>* node) {
	removeNode(node);
	insertFront(node);
}

template<typename K, typename V, typename Hash>
Node<K, V>* LRUCache<K, V, Hash>::popBack() {
	Node<K, V>* node = tail->prev;
	removeNode(node);

	return node;
}

// Lookup Helpers
template<typename K, typename V, typename Hash>
Node<K, V>* LRUCache<K, V, Hash>::findNode(const K& key) {
	auto it = cache.find(key);

	return it != cache.end() ? it->second : nullptr;
}

template<typename K, typename V, typename Hash>
const Node<K, V>* LRUCache<K, V, Hash>::findNode(const K& key) const {
	auto it = cache.find(key);

	return it != cache.end() ? it->second : nullptr;
}

// Resource Management
template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::release() noexcept {
	Node<K, V>* current = head;

	while (current) {
		Node<K, V>* next = current->next;

		delete current;

		current = next;
	}

	capacity_ = 0;
	head      = nullptr;
	tail      = nullptr;

	hitCounter  = 0;
	missCounter = 0;

	cache.clear();
}

// Lifecycle
template<typename K, typename V, typename Hash>
LRUCache<K, V, Hash>::LRUCache(std::size_t capacity) :
	capacity_(capacity),
	head(nullptr),
	tail(nullptr),
	hitCounter(0),
	missCounter(0)
{
	if (capacity == 0) {
		throw std::invalid_argument("LRUCache capacity must be greater than 0");
	}

	head = new Node<K, V>();
	tail = new Node<K, V>();

	head->next = tail;
	tail->prev = head;
}

template<typename K, typename V, typename Hash>
LRUCache<K, V, Hash>::~LRUCache() {
	release();
}

template<typename K, typename V, typename Hash>
LRUCache<K, V, Hash>::LRUCache(LRUCache<K, V, Hash>&& other) noexcept :
	capacity_(other.capacity_),
	head(other.head),
	tail(other.tail),
	hitCounter(other.hitCounter),
	missCounter(other.missCounter),
	cache(std::move(other.cache))
{
	other.capacity_   = 0;
	other.head        = nullptr;
	other.tail        = nullptr;
	other.hitCounter  = 0;
	other.missCounter = 0;
}

template<typename K, typename V, typename Hash>
LRUCache<K, V, Hash>& LRUCache<K, V, Hash>::operator=(LRUCache<K, V, Hash>&& other) noexcept {
	if (this != &other) {
		release();

		capacity_   = other.capacity_;
		head        = other.head;
		tail        = other.tail;
		hitCounter  = other.hitCounter;
		missCounter = other.missCounter;
		cache       = std::move(other.cache);

		other.capacity_   = 0;
		other.head        = nullptr;
		other.tail        = nullptr;
		other.hitCounter  = 0;
		other.missCounter = 0;
	}

	return *this;
}

// Cache Operations
template<typename K, typename V, typename Hash>
V* LRUCache<K, V, Hash>::get(const K& key) {
	Node<K, V>* node = findNode(key);

	if (node) {
		moveToFront(node);
		++hitCounter;

		return &node->value;
	}

	++missCounter;

	return nullptr;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::put(const K& key, const V& value) {
	Node<K, V>* node = findNode(key);

	if (node) {
		node->value = value;
		moveToFront(node);
		return;
	}

	if (cache.size() == capacity_) {
		Node<K, V>* lru = popBack();

		cache.erase(lru->key);

		delete lru;
	}

	Node<K, V>* newNode = new Node<K, V>(key, value);
	insertFront(newNode);
	cache[key] = newNode;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::put(const K& key, V&& value) {
	Node<K, V>* node = findNode(key);

	if (node) {
		node->value = std::move(value);
		moveToFront(node);
		return;
	}

	if (cache.size() == capacity_) {
		Node<K, V>* lru = popBack();

		cache.erase(lru->key);

		delete lru;
	}

	Node<K, V>* newNode = new Node<K, V>(key, std::move(value));
	insertFront(newNode);
	cache[key] = newNode;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::put(K&& key, V&& value) {
	Node<K, V>* node = findNode(key);

	if (node) {
		node->value = std::move(value);
		moveToFront(node);
		return;
	}

	if (cache.size() == capacity_) {
		Node<K, V>* lru = popBack();

		cache.erase(lru->key);

		delete lru;
	}

	K keyCopy = key;
	Node<K, V>* newNode = new Node<K, V>(std::move(key), std::move(value));
	insertFront(newNode);
	cache[std::move(keyCopy)] = newNode;
}

template<typename K, typename V, typename Hash>
bool LRUCache<K, V, Hash>::erase(const K& key) {
	Node<K, V>* node = findNode(key);

	if (node) {
		removeNode(node);

		cache.erase(node->key);

		delete node;

		return true;
	}

	return false;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::clear() {
	Node<K, V>* current = head->next;

	while (current != tail) {
		Node<K, V>* next = current->next;

		delete current;

		current = next;
	}

	head->next = tail;
	tail->prev = head;

	cache.clear();
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::resize(std::size_t newCapacity) {
	if (newCapacity == 0) {
		throw std::invalid_argument("LRUCache capacity must be greater than 0");
	}

	if (newCapacity <= cache.size()) {
		throw std::length_error("LRUCache newCapacity must be greater than current size");
	}

	capacity_ = newCapacity;
}

template<typename K, typename V, typename Hash>
bool LRUCache<K, V, Hash>::contains(const K& key) const {
	return cache.contains(key);
}

// Cache Access
template<typename K, typename V, typename Hash>
const V* LRUCache<K, V, Hash>::peek(const K& key) const {
	const Node<K, V>* node = findNode(key);

	return node ? &node->value : nullptr;
}

// LRU Order Inspection
template<typename K, typename V, typename Hash>
std::vector<K> LRUCache<K, V, Hash>::keys() const {
	std::vector<K> result;
	result.reserve(cache.size());

	Node<K, V>* current = head->next;

	while (current != tail) {
		result.push_back(current->key);
		current = current->next;
	}

	return result;
}

// Capacity Management
template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::reserve(std::size_t count) {
	cache.reserve(count);
}

// Cache Statistics
template<typename K, typename V, typename Hash>
std::size_t LRUCache<K, V, Hash>::hitCount() const noexcept {
	return hitCounter;
}

template<typename K, typename V, typename Hash>
std::size_t LRUCache<K, V, Hash>::missCount() const noexcept {
	return missCounter;
}

template<typename K, typename V, typename Hash>
double LRUCache<K, V, Hash>::hitRate() const noexcept {
	std::size_t total = hitCounter + missCounter;

	if (total == 0) {
		return 0.0;
	}

	return (static_cast<double>(hitCounter) / static_cast<double>(total)) * 100.0;
}

template<typename K, typename V, typename Hash>
void LRUCache<K, V, Hash>::resetStats() noexcept {
	hitCounter  = 0;
	missCounter = 0;
}

// LRU Order Inspection
template<typename K, typename V, typename Hash>
const K* LRUCache<K, V, Hash>::mostRecentKey() const {
	if (head->next == tail) {
		return nullptr;
	}

	return &head->next->key;
}

template<typename K, typename V, typename Hash>
const K* LRUCache<K, V, Hash>::leastRecentKey() const {
	if (tail->prev == head) {
		return nullptr;
	}

	return &tail->prev->key;
}

// Capacity
template<typename K, typename V, typename Hash>
std::size_t LRUCache<K, V, Hash>::capacity() const noexcept {
	return capacity_;
}

template<typename K, typename V, typename Hash>
std::size_t LRUCache<K, V, Hash>::size() const noexcept {
	return cache.size();
}

// State

template<typename K, typename V, typename Hash>
bool LRUCache<K, V, Hash>::empty() const noexcept {
	return cache.empty();
}
