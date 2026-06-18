#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "Node.h"

template<
    typename K,
    typename V,
    bool IsConst = false
>
class Iterator {
private:
    // Type Aliases
    using node = hashmap::Node<K, V>;

public:
    // Iterator Traits
    using value_type = node;
    using difference_type = std::ptrdiff_t;

    using pointer = std::conditional_t<
        IsConst,
        const node*,
        node*
    >;

    using reference = std::conditional_t<
        IsConst,
        const node&,
        node&
    >;

    using iterator_category = std::bidirectional_iterator_tag;

    using BucketPointer = std::conditional_t<
        IsConst,
        node* const*,
        node**
    >;

private:
    // Core State
    pointer current = nullptr;
    BucketPointer buckets = nullptr;
    std::size_t bucketIndex = 0;
    std::size_t bucketCount = 0;

public:
    // Lifecycle
    Iterator() = default;

    Iterator(pointer nodePtr,
             BucketPointer bucketPtr,
             std::size_t index,
             std::size_t count)
        : current(nodePtr)
        , buckets(bucketPtr)
        , bucketIndex(index)
        , bucketCount(count)
    {}

    // Dereferencing
    [[nodiscard]] reference operator*() const {
        return *current;
    }

    [[nodiscard]] pointer operator->() const {
        return current;
    }

    // Forward Iteration
    Iterator& operator++() {
        if (current && current->next) {
            current = current->next;
            return *this;
        }

        ++bucketIndex;

        while (bucketIndex < bucketCount) {
            if (buckets[bucketIndex]) {
                current = buckets[bucketIndex];
                return *this;
            }

            ++bucketIndex;
        }

        current = nullptr;
        return *this;
    }

    Iterator operator++(int) {
        Iterator temp(*this);
        ++(*this);
        return temp;
    }

    // Reverse Iteration
    Iterator& operator--() {
        if (current == nullptr) {
            for (std::size_t i = bucketCount; i > 0; --i) {
                node* last = buckets[i - 1];

                if (!last)
                    continue;

                while (last->next)
                    last = last->next;

                current = last;
                bucketIndex = i - 1;
                return *this;
            }

            return *this;
        }

        node* first = buckets[bucketIndex];

        if (first != current) {
            while (first->next != current)
                first = first->next;

            current = first;
            return *this;
        }

        for (std::size_t i = bucketIndex; i > 0; --i) {
            node* last = buckets[i - 1];

            if (!last)
                continue;

            while (last->next)
                last = last->next;

            current = last;
            bucketIndex = i - 1;
            return *this;
        }

        current = nullptr;
        bucketIndex = 0;
        return *this;
    }

    Iterator operator--(int) {
        Iterator temp(*this);
        --(*this);
        return temp;
    }

    // Comparison
    [[nodiscard]] bool operator==(const Iterator& other) const {
        return current == other.current;
    }

    [[nodiscard]] bool operator!=(const Iterator& other) const {
        return !(*this == other);
    }

    // Const Conversion
    operator Iterator<K, V, true>() const {
        return Iterator<K, V, true>(
            current,
            buckets,
            bucketIndex,
            bucketCount
        );
    }
};