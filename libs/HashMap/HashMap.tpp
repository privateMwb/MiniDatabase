#include <cstddef>
#include <stdexcept>
#include <new>

// Lifecyle
template<typename K,
         typename V,
         typename Hash>
HashMap<K, V, Hash>::HashMap(std::size_t bucketCount)
	: bucketCount(bucketCount == 0 ? 1 : bucketCount)
	, elementCount(0)
	, buckets(nullptr)
	, hasher(Hash()) {
	buckets = new node*[bucketCount];

	for(std::size_t i = 0; i < bucketCount; ++i) {
		buckets[i] = nullptr;
	}
}

template<typename K,
         typename V,
         typename Hash>
HashMap<K, V, Hash>::~HashMap() {
	release();
}

template<typename K,
         typename V,
         typename Hash>
HashMap<K, V, Hash>::HashMap(const HashMap<K, V, Hash>& other)
	: buckets(new node*[other.bucketCount])
	, bucketCount(other.bucketCount)
	, elementCount(0)
	, hasher(other.hasher)
{
	for(std::size_t i = 0; i < other.bucketCount; ++i) {
		buckets[i] = nullptr;
	}

	for(std::size_t i = 0; i < other.bucketCount; ++i) {
		node* current = other.buckets[i];

		while(current) {
			insert(current->key, current->value);

			current = current->next;
		}
	}
}

template<typename K,
         typename V,
         typename Hash>
HashMap<K, V, Hash>& HashMap<K, V, Hash>::operator=(const HashMap<K, V, Hash>& other) {
	if(this != &other) {
		release();

		bucketCount = other.bucketCount;
		elementCount = 0;
		hasher = other.hasher;
		buckets = new node*[bucketCount];

		for(std::size_t i = 0; i < other.bucketCount; ++i) {
			buckets[i] = nullptr;
		}

		for(std::size_t i = 0; i < other.bucketCount; ++i) {
			node* current = other.buckets[i];

			while(current) {
				insert(current->key, current->value);

				current = current->next;
			}
		}
	}

	return *this;
}

template<typename K,
         typename V,
         typename Hash>
HashMap<K, V, Hash>::HashMap(HashMap<K, V, Hash>&& other) noexcept
	: buckets(other.buckets)
	, bucketCount(other.bucketCount)
	, elementCount(other.elementCount)
	, hasher(std::move(other.hasher))
{
	other.buckets = nullptr;
	other.bucketCount = 0;
	other.elementCount = 0;
}

template<typename K,
         typename V,
         typename Hash>
HashMap<K, V, Hash>& HashMap<K, V, Hash>::operator=(HashMap<K, V, Hash>&& other) noexcept {
	if(this != &other) {
		release();

		buckets = other.buckets;
		bucketCount = other.bucketCount;
		elementCount = other.elementCount;
		hasher = std::move(other.hasher);

		other.buckets = nullptr;
		other.bucketCount = 0;
		other.elementCount = 0;
	}

	return *this;
}

// Private Helper
template<typename K,
         typename V,
         typename Hash>
std::size_t HashMap<K, V, Hash>::bucketIndex(const K& key) const {
	return hasher(key) % bucketCount;
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::node*
HashMap<K, V, Hash>::findNode(const K& key) {
    std::size_t index = bucketIndex(key);

    node* current = buckets[index];

    while (current) {
        if (current->key == key)
            return current;

        current = current->next;
    }

    return nullptr;
}

template<typename K,
         typename V,
         typename Hash>
void HashMap<K, V, Hash>::rehash(std::size_t newBucketCount) {
	if(newBucketCount <= bucketCount) return;

	node** newBuckets = new node*[newBucketCount];

	for(std::size_t i = 0; i < newBucketCount; ++i) {
		newBuckets[i] = nullptr;
	}

	for(std::size_t i = 0; i < bucketCount; ++i) {
		node* current = buckets[i];

		while(current) {
			node* next = current->next;

			std::size_t index = hasher(current->key) % newBucketCount;

			current->next = newBuckets[index];
			newBuckets[index] = current;

			current = next;
		}
	}

	delete[] buckets;

	buckets = newBuckets;
	bucketCount = newBucketCount;
}

template<typename K,
         typename V,
         typename Hash>
const typename HashMap<K, V, Hash>::node*
HashMap<K, V, Hash>::findNode(const K& key) const {
    std::size_t index = bucketIndex(key);

    const node* current = buckets[index];

    while (current) {
        if (current->key == key)
            return current;

        current = current->next;
    }

    return nullptr;
}


template<typename K,
         typename V,
         typename Hash>
void HashMap<K, V, Hash>::release() noexcept {
	if(buckets == nullptr) {
		return;
	}

	for(std::size_t i = 0; i < bucketCount; ++i) {
		node* current = buckets[i];

		while(current) {
			node* next = current->next;

			delete current;

			current = next;
		}
	}

	delete[] buckets;

	buckets = nullptr;
	bucketCount = 0;
	elementCount = 0;
}

// Access
template<typename K,
         typename V,
         typename Hash>
V& HashMap<K, V, Hash>::operator[](const K& key) {
	node* node = findNode(key);

	if(node == nullptr) {
		insert(key, V{});
		node = findNode(key);
	}

	return node->value;
}

template<typename K,
         typename V,
         typename Hash>
V& HashMap<K, V, Hash>::at(const K& key) {
	node* node = findNode(key);

	if(node == nullptr)
		throw std::out_of_range("HashMap::at: key not found");

	return node->value;
}

template<typename K,
         typename V,
         typename Hash>
const V& HashMap<K, V, Hash>::at(const K& key) const {
	const node* node = findNode(key);

	if(node == nullptr)
		throw std::out_of_range("HashMap::at: key not found");

	return node->value;
}

// Modifiers
template<typename K,
         typename V,
         typename Hash>
void HashMap<K, V, Hash>::insert(const K& key, const V& value) {
	if(findNode(key) != nullptr) return;

	std::size_t index = bucketIndex(key);

	node* newNode = new node(key, value);

	newNode->next = buckets[index];
	buckets[index] = newNode;

	++elementCount;

	if(static_cast<double>(elementCount) / bucketCount > MAX_LOAD_FACTOR) {
		rehash(bucketCount * 2);
	}
}

template<typename K,
         typename V,
         typename Hash>
bool HashMap<K, V, Hash>::update(const K& key, const V& value) {
	node* node = findNode(key);

	if(node == nullptr) return false;

	node->value = value;
	return true;
}

template<typename K,
         typename V,
         typename Hash>
bool HashMap<K, V, Hash>::erase(const K& key) {
	std::size_t index = bucketIndex(key);

	node* current = buckets[index];
	node* prev = nullptr;

	while(current) {
		if(current->key == key) {
			if(prev == nullptr) {
				buckets[index] = current->next;
			} else {
				prev->next = current->next;
			}

			delete current;
			--elementCount;

			return true;
		}

		prev = current;
		current = current->next;
	}

	return false;
}

template<typename K,
         typename V,
         typename Hash>
void HashMap<K, V, Hash>::clear() {
	for(std::size_t i = 0; i < bucketCount; ++i) {
		node* current = buckets[i];

		while(current) {
			node* next = current->next;

			delete current;

			current = next;
		}

		buckets[i] = nullptr;
	}

	elementCount = 0;
}

// Lookup
template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::iterator HashMap<K, V, Hash>::find(const K& key) {
	node* node = findNode(key);

	if(node == nullptr) {
		return end();
	}

	return iterator(
	           node,
	           buckets,
	           bucketIndex(key),
	           bucketCount
	       );
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_iterator HashMap<K, V, Hash>::find(const K& key) const {
	const node* node = findNode(key);

	if(node == nullptr) {
		return end();
	}

	return const_iterator(
	           node,
	           buckets,
	           bucketIndex(key),
	           bucketCount
	       );
}

template<typename K,
         typename V,
         typename Hash>
bool HashMap<K, V, Hash>::contains(const K& key) const noexcept {
	return findNode(key) != nullptr;
}

// Capacity
template<typename K,
         typename V,
         typename Hash>
std::size_t HashMap<K, V, Hash>::capacity() const noexcept {
	return bucketCount;
}

template<typename K,
         typename V,
         typename Hash>
std::size_t HashMap<K, V, Hash>::size() const noexcept {
	return elementCount;
}

// State
template<typename K,
         typename V,
         typename Hash>
bool HashMap<K, V, Hash>::empty() const noexcept {
	return elementCount == 0;
}

// Iterators
template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::iterator HashMap<K, V, Hash>::begin() noexcept {
	for(std::size_t i = 0; i < bucketCount; ++i) {
		if(buckets[i] != nullptr) {
			return iterator(
			           buckets[i],
			           buckets,
			           i,
			           bucketCount
			       );
		}
	}

	return end();
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::iterator HashMap<K, V, Hash>::end() noexcept {
	return iterator(
	           nullptr,
	           buckets,
	           0,
	           bucketCount
	       );
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_iterator HashMap<K, V, Hash>::begin() const noexcept {
	return const_iterator(begin());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_iterator HashMap<K, V, Hash>::end() const noexcept {
	return const_iterator(end());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_iterator HashMap<K, V, Hash>::cbegin() const noexcept {
	return const_iterator(begin());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_iterator HashMap<K, V, Hash>::cend() const noexcept {
	return const_iterator(end());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::reverse_iterator HashMap<K, V, Hash>::rbegin() noexcept {
	return reverse_iterator(end());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::reverse_iterator HashMap<K, V, Hash>::rend() noexcept {
	return reverse_iterator(begin());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_reverse_iterator HashMap<K, V, Hash>::rbegin() const noexcept {
	return const_reverse_iterator(cbegin());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_reverse_iterator HashMap<K, V, Hash>::rend() const noexcept {
	return const_reverse_iterator(cend());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_reverse_iterator HashMap<K, V, Hash>::crbegin() const noexcept {
	return const_reverse_iterator(cbegin());
}

template<typename K,
         typename V,
         typename Hash>
typename HashMap<K, V, Hash>::const_reverse_iterator HashMap<K, V, Hash>::crend() const noexcept {
	return const_reverse_iterator(cend());
}


