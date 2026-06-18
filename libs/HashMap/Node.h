#pragma once
namespace hashmap {
template<typename K, typename V>
struct Node {
	const K key;
	V value;

	Node* next = nullptr;

	Node(const K& key, const V& value)
		: key(key)
		, value(value)
	{}
};
}