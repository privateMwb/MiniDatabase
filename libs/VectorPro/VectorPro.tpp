// Data Management
template<typename T>
void VectorPro<T>::release() {
    if (data) {
        for (size_type i = 0; i < v_size; ++i)
            data[i].~T();

        ::operator delete(data);

        data   = nullptr;
        v_size = 0;
        v_cap  = 0;
    }

    if (listeners) {
        delete[] listeners;

        listeners = nullptr;
        l_size    = 0;
        l_cap     = 0;
    }
}

template<typename T>
void VectorPro<T>::reallocateData(size_type newCap) {
    T*        newData     = static_cast<T*>(::operator new(sizeof(T) * newCap));
    size_type old_v_size  = v_size;
    size_type i           = 0;

    try {
        for (; i < old_v_size; ++i)
            new (newData + i) T(std::move_if_noexcept(data[i]));
    } catch (...) {
        for (size_type j = 0; j < i; ++j)
            newData[j].~T();

        ::operator delete(newData);
        throw;
    }

    for (size_type k = 0; k < old_v_size; ++k)
        data[k].~T();

    ::operator delete(data);

    data  = newData;
    v_cap = newCap;
}

template<typename T>
void VectorPro<T>::reallocateListener(size_type newCap) {
    Listener* newListeners = new Listener[newCap];

    for (size_type i = 0; i < l_size; ++i)
        newListeners[i] = listeners[i];

    delete[] listeners;

    listeners = newListeners;
    l_cap     = newCap;
}

template<typename T>
typename VectorPro<T>::size_type VectorPro<T>::growData() {
    return v_cap == 0 ? INITIAL_CAP : v_cap + std::max(v_cap / 2, size_type(1));
}

template<typename T>
typename VectorPro<T>::size_type VectorPro<T>::growListener() {
    return l_cap == 0 ? INITIAL_CAP : l_cap + std::max(l_cap / 2, size_type(1));
}

template<typename T>
void VectorPro<T>::notify(EventType type) {
    if (l_size == 0) [[unlikely]] return;

    for (size_type i = 0; i < l_size; ++i) [[likely]]
        listeners[i](*this, type);
}

// Constructors & Destructor
template<typename T>
VectorPro<T>::VectorPro(std::initializer_list<T> init) {
    reserve(init.size());

    for (const auto& value : init)
        push_back(value);
}

template<typename T>
VectorPro<T>::~VectorPro() noexcept {
    release();
}

template<typename T>
VectorPro<T>::VectorPro(const VectorPro<T>& other)
    : data(static_cast<T*>(::operator new(sizeof(T) * other.v_cap)))
    , v_size(other.v_size)
    , v_cap(other.v_cap)
    , listeners(new Listener[other.l_cap])
    , l_size(other.l_size)
    , l_cap(other.l_cap)
{
    size_type i = 0;

    try {
        for (; i < v_size; ++i)
            new (data + i) T(other.data[i]);

        for (size_type j = 0; j < l_size; ++j)
            listeners[j] = other.listeners[j];
    } catch (...) {
        for (size_type k = 0; k < i; ++k)
            data[k].~T();

        ::operator delete(data);
        delete[] listeners;

        throw;
    }
}

template<typename T>
VectorPro<T>& VectorPro<T>::operator=(const VectorPro<T>& other) {
    if (this != &other) {
        VectorPro<T> temp(other);
        swap(*this, temp);
    }

    return *this;
}

template<typename T>
VectorPro<T>::VectorPro(VectorPro<T>&& other) noexcept
    : data(other.data)
    , v_size(other.v_size)
    , v_cap(other.v_cap)
    , listeners(other.listeners)
    , l_size(other.l_size)
    , l_cap(other.l_cap)
{
    other.data      = nullptr;
    other.v_size    = 0;
    other.v_cap     = 0;
    other.listeners = nullptr;
    other.l_size    = 0;
    other.l_cap     = 0;
}

template<typename T>
VectorPro<T>& VectorPro<T>::operator=(VectorPro<T>&& other) noexcept {
    if (this != &other) {
        clear();
        ::operator delete(data);

        delete[] listeners;

        data      = other.data;
        v_size    = other.v_size;
        v_cap     = other.v_cap;
        listeners = other.listeners;
        l_size    = other.l_size;
        l_cap     = other.l_cap;

        other.data      = nullptr;
        other.v_size    = 0;
        other.v_cap     = 0;
        other.listeners = nullptr;
        other.l_size    = 0;
        other.l_cap     = 0;
    }
    return *this;
}
// Modifiers
template<typename T>
void VectorPro<T>::push_back(const T& value) {
    if (v_size == v_cap) [[unlikely]] reallocateData(growData());

    new (data + v_size) T(value);
    ++v_size;

    notify(EventType::PushBack);
}

template<typename T>
void VectorPro<T>::push_back(T&& value) {
    if (v_size == v_cap) [[unlikely]] reallocateData(growData());

    new (data + v_size) T(std::move(value));
    ++v_size;

    notify(EventType::PushBack);
}

template<typename T>
template<typename... Args>
void VectorPro<T>::emplace_back(Args&&... args) {
    if (v_size == v_cap) [[unlikely]] reallocateData(growData());

    new (data + v_size) T(std::forward<Args>(args)...);
    ++v_size;

    notify(EventType::EmplaceBack);
}

template<typename T>
void VectorPro<T>::insert(size_type pos, const T& value) {
    if (pos > v_size) throw std::out_of_range("VectorPro::insert position out of range");

    if (v_size == v_cap) [[unlikely]] reallocateData(growData());

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memmove(data + pos + 1, data + pos, (v_size - pos) * sizeof(T));
    } else {
        for (size_type i = v_size; i > pos; --i) {
            new (data + i) T(std::move_if_noexcept(data[i - 1]));
            data[i - 1].~T();
        }
    }

    new (data + pos) T(value);
    ++v_size;

    notify(EventType::Insert);
}

template<typename T>
void VectorPro<T>::insert(size_type pos, T&& value) {
    if (pos > v_size) throw std::out_of_range("VectorPro::insert position out of range");

    if (v_size == v_cap) [[unlikely]] reallocateData(growData());

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memmove(data + pos + 1, data + pos, (v_size - pos) * sizeof(T));
    } else {
        for (size_type i = v_size; i > pos; --i) {
            new (data + i) T(std::move_if_noexcept(data[i - 1]));
            data[i - 1].~T();
        }
    }

    new (data + pos) T(std::move(value));
    ++v_size;

    notify(EventType::Insert);
}

template<typename T>
template<typename Predicate>
void VectorPro<T>::remove_if(Predicate pred) {
    if (v_size == 0) return;

    size_type write = 0;

    for (size_type read = 0; read < v_size; ++read) {
        if (!pred(data[read])) {
            if (write != read) {
                new (data + write) T(std::move_if_noexcept(data[read]));
                data[read].~T();
            }
            ++write;
        } else {
            data[read].~T();
        }
    }

    v_size = write;

    notify(EventType::Remove);
}

template<typename T>
void VectorPro<T>::pop_back() {
    if (v_size == 0) return;

    data[v_size - 1].~T();
    --v_size;

    notify(EventType::PopBack);
}

template<typename T>
void VectorPro<T>::erase(size_type index) {
    if (index >= v_size) throw std::out_of_range("VectorPro::erase index out of range");

    data[index].~T();

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memmove(data + index, data + index + 1, (v_size - index - 1) * sizeof(T));
    } else {
        for (size_type i = index; i < v_size - 1; ++i) {
            new (data + i) T(std::move_if_noexcept(data[i + 1]));
            data[i + 1].~T();
        }
    }

    --v_size;

    notify(EventType::Erase);
}

template<typename T>
void VectorPro<T>::clear() {
    if (v_size == 0) return;

    for (size_type i = 0; i < v_size; ++i)
        data[i].~T();

    v_size = 0;

    notify(EventType::Clear);
}

template<typename T>
void VectorPro<T>::reserve(size_type newCap) {
    if (newCap <= v_cap) return;

    reallocateData(newCap);

    notify(EventType::Reserve);
}

template<typename T>
void VectorPro<T>::shrink_to_fit() {
    if (v_size == v_cap) return;

    if (v_size == 0) {
        ::operator delete(data);

        data  = nullptr;
        v_cap = 0;

        notify(EventType::Shrink);
        return;
    }

    T*        newData = static_cast<T*>(::operator new(sizeof(T) * v_size));
    size_type i       = 0;

    try {
        for (; i < v_size; ++i)
            new (newData + i) T(std::move_if_noexcept(data[i]));
    } catch (...) {
        for (size_type j = 0; j < i; ++j)
            newData[j].~T();

        ::operator delete(newData);
        throw;
    }

    for (size_type k = 0; k < v_size; ++k)
        data[k].~T();

    ::operator delete(data);

    data  = newData;
    v_cap = v_size;

    notify(EventType::Shrink);
}

// Observer
template<typename T>
void VectorPro<T>::subscribe(Listener listener) {
    if (l_size == l_cap) [[unlikely]] reallocateListener(growListener());

    listeners[l_size] = listener;
    ++l_size;
}

template<typename T>
void VectorPro<T>::unsubscribe(size_type index) {
    if (index >= l_size) throw std::out_of_range("VectorPro::unsubscribe index out of range");

    for (size_type i = index; i < l_size - 1; ++i)
        listeners[i] = listeners[i + 1];

    --l_size;
}

// Capacity
template<typename T>
bool VectorPro<T>::empty() const noexcept {
    return v_size == 0;
}

template<typename T>
typename VectorPro<T>::size_type VectorPro<T>::size() const noexcept {
    return v_size;
}

template<typename T>
typename VectorPro<T>::size_type VectorPro<T>::capacity() const noexcept {
    return v_cap;
}

// Element Access
template<typename T>
T* VectorPro<T>::data_ptr() noexcept {
    return data;
}

template<typename T>
const T* VectorPro<T>::data_ptr() const noexcept {
    return data;
}

template<typename T>
T& VectorPro<T>::operator[](size_type index) noexcept {
    return data[index];
}

template<typename T>
const T& VectorPro<T>::operator[](size_type index) const noexcept {
    return data[index];
}

template<typename T>
T& VectorPro<T>::at(size_type index) {
    if (index >= v_size) throw std::out_of_range("VectorPro::at index out of range");

    return data[index];
}

template<typename T>
const T& VectorPro<T>::at(size_type index) const {
    if (index >= v_size) throw std::out_of_range("VectorPro::at index out of range");

    return data[index];
}

template<typename T>
T& VectorPro<T>::front() {
    if (v_size == 0) throw std::out_of_range("VectorPro::front on empty vector");

    return data[0];
}

template<typename T>
const T& VectorPro<T>::front() const {
    if (v_size == 0) throw std::out_of_range("VectorPro::front on empty vector");

    return data[0];
}

template<typename T>
T& VectorPro<T>::back() {
    if (v_size == 0) throw std::out_of_range("VectorPro::back on empty vector");

    return data[v_size - 1];
}

template<typename T>
const T& VectorPro<T>::back() const {
    if (v_size == 0) throw std::out_of_range("VectorPro::back on empty vector");

    return data[v_size - 1];
}

// Iterators
template<typename T>
typename VectorPro<T>::iterator VectorPro<T>::begin() noexcept {
    return iterator(data);
}

template<typename T>
typename VectorPro<T>::iterator VectorPro<T>::end() noexcept {
    return iterator(data + v_size);
}

template<typename T>
typename VectorPro<T>::const_iterator VectorPro<T>::begin() const noexcept {
    return const_iterator(data);
}

template<typename T>
typename VectorPro<T>::const_iterator VectorPro<T>::end() const noexcept {
    return const_iterator(data + v_size);
}

template<typename T>
typename VectorPro<T>::const_iterator VectorPro<T>::cbegin() const noexcept {
    return const_iterator(data);
}

template<typename T>
typename VectorPro<T>::const_iterator VectorPro<T>::cend() const noexcept {
    return const_iterator(data + v_size);
}

template<typename T>
typename VectorPro<T>::reverse_iterator VectorPro<T>::rbegin() noexcept {
    return reverse_iterator(end());
}

template<typename T>
typename VectorPro<T>::reverse_iterator VectorPro<T>::rend() noexcept {
    return reverse_iterator(begin());
}

template<typename T>
typename VectorPro<T>::const_reverse_iterator VectorPro<T>::rbegin() const noexcept {
    return const_reverse_iterator(end());
}

template<typename T>
typename VectorPro<T>::const_reverse_iterator VectorPro<T>::rend() const noexcept {
    return const_reverse_iterator(begin());
}

template<typename T>
typename VectorPro<T>::const_reverse_iterator VectorPro<T>::crbegin() const noexcept {
    return const_reverse_iterator(cend());
}

template<typename T>
typename VectorPro<T>::const_reverse_iterator VectorPro<T>::crend() const noexcept {
    return const_reverse_iterator(cbegin());
}
