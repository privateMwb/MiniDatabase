#pragma once

#include "Iterator.h"

#include <iterator>
#include <stdexcept>
#include <cstddef>
#include <utility>
#include <cstring>
#include <type_traits>
#include <initializer_list>

template<typename T>
class VectorPro {
public:
    // Types
    using size_type = std::size_t;

    enum class EventType {
        PushBack,
        EmplaceBack,
        PopBack,
        Insert,
        Remove,
        Erase,
        Clear,
        Shrink,
        Reserve
    };

    using Listener = void(*)(const VectorPro<T>&, EventType);

    using iterator               = vectorpro::Iterator<T>;
    using const_iterator         = vectorpro::Iterator<const T>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    // Data
    T*        data     = nullptr;
    size_type v_size   = 0;
    size_type v_cap    = 0;

    // Observer
    Listener* listeners = nullptr;
    size_type l_size    = 0;
    size_type l_cap     = 0;

    // Config
    static constexpr size_type INITIAL_CAP = 8;

    // Data Management
    void release();

    [[nodiscard]] size_type growData();
    [[nodiscard]] size_type growListener();

    void reallocateData(size_type newCap);
    void reallocateListener(size_type newCap);

    // Internal — fires all registered listeners with the given event
    void notify(EventType type);

public:
    // Constructors & Destructor
    VectorPro() = default;
    VectorPro(std::initializer_list<T> init);

    ~VectorPro() noexcept;

    VectorPro(const VectorPro& other);
    VectorPro& operator=(const VectorPro& other);

    VectorPro(VectorPro&& other) noexcept;
    VectorPro& operator=(VectorPro&& other) noexcept;

    // Modifiers
    void push_back(const T& value);
    void push_back(T&& value);

    template<typename... Args>
    void emplace_back(Args&&... args);

    // pos must be <= size()
    void insert(size_type pos, const T& value);
    void insert(size_type pos, T&& value);

    template<typename Predicate>
    void remove_if(Predicate pred);

    void pop_back();

    // index must be < size()
    void erase(size_type index);

    void clear();

    void reserve(size_type newCap);
    void shrink_to_fit();

    // Observer
    // listener must match signature: void(const VectorPro<T>&, EventType)
    void subscribe(Listener listener);
    void unsubscribe(size_type index);

    // Swap
    friend void swap(VectorPro& a, VectorPro& b) noexcept {
        using std::swap;
        swap(a.data,      b.data);
        swap(a.v_size,    b.v_size);
        swap(a.v_cap,     b.v_cap);
        swap(a.listeners, b.listeners);
        swap(a.l_size,    b.l_size);
        swap(a.l_cap,     b.l_cap);
    }

    // Capacity
    [[nodiscard]] bool      empty()    const noexcept;
    [[nodiscard]] size_type size()     const noexcept;
    [[nodiscard]] size_type capacity() const noexcept;

    // Element Access
    [[nodiscard]] T*       data_ptr()       noexcept;
    [[nodiscard]] const T* data_ptr() const noexcept;

    [[nodiscard]] T&       at(size_type index);
    [[nodiscard]] const T& at(size_type index) const;

    [[nodiscard]] T&       front();
    [[nodiscard]] const T& front() const;

    [[nodiscard]] T&       back();
    [[nodiscard]] const T& back() const;

    [[nodiscard]] T&       operator[](size_type index) noexcept;
    [[nodiscard]] const T& operator[](size_type index) const noexcept;

    // Iterators
    [[nodiscard]] iterator               begin()   noexcept;
    [[nodiscard]] iterator               end()     noexcept;
    [[nodiscard]] const_iterator         begin()   const noexcept;
    [[nodiscard]] const_iterator         end()     const noexcept;
    [[nodiscard]] const_iterator         cbegin()  const noexcept;
    [[nodiscard]] const_iterator         cend()    const noexcept;
    [[nodiscard]] reverse_iterator       rbegin()  noexcept;
    [[nodiscard]] reverse_iterator       rend()    noexcept;
    [[nodiscard]] const_reverse_iterator rbegin()  const noexcept;
    [[nodiscard]] const_reverse_iterator rend()    const noexcept;
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept;
    [[nodiscard]] const_reverse_iterator crend()   const noexcept;
};

#include "VectorPro.tpp"
