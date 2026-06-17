#pragma once

#include <iterator>
#include <cstddef>
#include <type_traits>

namespace vectorpro {

template<typename T>
class Iterator {
private:
    T* ptr;

public:
    // Types
    using value_type        = std::remove_const_t<T>;
    using pointer           = T*;
    using reference         = T&;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept  = std::contiguous_iterator_tag;

    // Constructor
    constexpr explicit Iterator(T* p = nullptr) noexcept : ptr(p) {}

    // Conversion — allows Iterator<T> -> Iterator<const T>
    template<typename U>
    requires std::convertible_to<U*, T*>
    constexpr Iterator(const Iterator<U>& other) noexcept : ptr(other.base()) {}

    // Base
    [[nodiscard]]
    constexpr T* base() const noexcept {
        return ptr;
    }

    // Dereference
    [[nodiscard]]
    constexpr reference operator*() const noexcept {
        return *ptr;
    }

    [[nodiscard]]
    constexpr pointer operator->() const noexcept {
        return ptr;
    }

    // Increment / Decrement
    constexpr Iterator& operator++() noexcept {
        ++ptr;
        return *this;
    }

    constexpr Iterator& operator--() noexcept {
        --ptr;
        return *this;
    }

    constexpr Iterator operator++(int) noexcept {
        Iterator tmp(*this);
        ++ptr;
        return tmp;
    }

    constexpr Iterator operator--(int) noexcept {
        Iterator tmp(*this);
        --ptr;
        return tmp;
    }

    // Arithmetic
    constexpr Iterator& operator+=(difference_type n) noexcept {
        ptr += n;
        return *this;
    }

    constexpr Iterator& operator-=(difference_type n) noexcept {
        ptr -= n;
        return *this;
    }

    [[nodiscard]]
    constexpr Iterator operator+(difference_type n) const noexcept {
        return Iterator(ptr + n);
    }

    [[nodiscard]]
    constexpr Iterator operator-(difference_type n) const noexcept {
        return Iterator(ptr - n);
    }

    // Iterator Difference
    template<typename U>
    [[nodiscard]]
    constexpr difference_type operator-(const Iterator<U>& other) const noexcept {
        return ptr - other.base();
    }

    // Indexing
    [[nodiscard]]
    constexpr reference operator[](difference_type n) const noexcept {
        return *(ptr + n);
    }

    // Comparison
    template<typename U>
    [[nodiscard]]
    constexpr auto operator<=>(const Iterator<U>& other) const noexcept {
        return ptr <=> other.base();
    }

    template<typename U>
    [[nodiscard]]
    constexpr bool operator==(const Iterator<U>& other) const noexcept {
        return ptr == other.base();
    }

    template<typename U>
    [[nodiscard]]
    constexpr bool operator!=(const Iterator<U>& other) const noexcept {
        return ptr != other.base();
    }
};

// Arithmetic (free function) — supports n + iterator
template<typename T>
[[nodiscard]]
constexpr Iterator<T> operator+(std::ptrdiff_t n, const Iterator<T>& it) noexcept {
    return it + n;
}

/*
// to_address — enables std::contiguous_iterator compatibility with std::span and std::ranges
template<typename T>
constexpr T* std::to_address(const Iterator<T>& it) noexcept {
    return it.base();
}
*/
}
