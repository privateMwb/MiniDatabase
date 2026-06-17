#pragma once

#include <utility>

template<typename K, typename V>
struct Node {
    K key;
    V value;

    Node* prev = nullptr;
    Node* next = nullptr;

    Node() = default;

    explicit Node(const K& key, const V& value) :
        key(key),
        value(value) {}

    Node(const K& key, V&& value) :
        key(key),
        value(std::move(value)) {}

    Node(K&& key, V&& value) :
        key(std::move(key)),
        value(std::move(value)) {}
};
