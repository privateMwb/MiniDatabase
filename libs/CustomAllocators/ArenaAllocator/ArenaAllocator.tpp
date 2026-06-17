#pragma once

#include <cassert>

// Alignment Utilities
constexpr bool ArenaAllocator::isPowerOfTwo(std::size_t alignment) noexcept {
	return alignment != 0 && (alignment & (alignment - 1)) == 0;
}

constexpr std::size_t ArenaAllocator::alignForward(std::size_t value, std::size_t alignment) noexcept {
	return (value + alignment - 1) & ~(alignment - 1);
}

// Constructors & Destructor
inline ArenaAllocator::ArenaAllocator(std::size_t size)
	: memory(static_cast<std::byte*>(
	             ::operator new(size, std::align_val_t{alignof(std::max_align_t)})))
	, cap(size)
	, offset(0)
	, frameDepth(0)
	, stats{}
{}

inline ArenaAllocator::~ArenaAllocator() {
	::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}

inline ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept
	: memory(other.memory)
	, cap(other.cap)
	, offset(other.offset)
	, frameDepth(other.frameDepth)
	, stats(other.stats)
{
	frameStack = std::move(other.frameStack);

	other.memory     = nullptr;
	other.cap        = 0;
	other.offset     = 0;
	other.frameDepth = 0;
	other.stats      = {};
}

inline ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {
	if (this != &other) {
		::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});

		memory     = other.memory;
		cap        = other.cap;
		offset     = other.offset;
		frameDepth = other.frameDepth;
		stats      = other.stats;

		frameStack = std::move(other.frameStack);

		other.memory     = nullptr;
		other.cap        = 0;
		other.offset     = 0;
		other.frameDepth = 0;
		other.stats      = {};
	}
	return *this;
}

// Core Allocation
inline void* ArenaAllocator::allocate(std::size_t size, std::size_t alignment) noexcept {
	assert(isPowerOfTwo(alignment) && "alignment must be a non-zero power of two");
	assert(size > 0 && "allocation size must be > 0");

	const std::size_t aligned = alignForward(offset, alignment);

	if (aligned + size > cap) [[unlikely]]
		return nullptr;

	void* ptr = memory + aligned;
	offset = aligned + size;

	++stats.allocations;
	stats.totalAllocated += size;
	stats.currentUsed = offset;

	if (stats.currentUsed > stats.peakUsed) [[unlikely]]
		stats.peakUsed = stats.currentUsed;

	return ptr;
}

template<typename T>
[[nodiscard]] T* ArenaAllocator::allocate() noexcept {
	return static_cast<T*>(allocate(sizeof(T), alignof(T)));
}

// Object Lifecycle
template<typename T, typename... Args>
T* ArenaAllocator::create(Args&&... args) {
	void* raw = allocate(sizeof(T), alignof(T));

	if (!raw) [[unlikely]]
		return nullptr;

	return ::new (raw) T(std::forward<Args>(args)...);
}

template<typename T>
void ArenaAllocator::destroy(T* ptr) noexcept {
	if (!ptr || !owns(ptr))
		return;

	ptr->~T();
}

// Frame Management
inline void ArenaAllocator::beginFrame() noexcept {
	assert(frameDepth < kMaxFrameDepth && "ArenaAllocator: frame stack overflow");
	frameStack[frameDepth++] = offset;
}

inline void ArenaAllocator::endFrame() noexcept {
	assert(frameDepth > 0 && "ArenaAllocator: endFrame() without matching beginFrame()");
	offset = frameStack[--frameDepth];
	stats.currentUsed = offset;
}

// State Management
inline void ArenaAllocator::reset() noexcept {
	offset = 0;
	frameDepth = 0;
	stats.currentUsed = 0;
	stats.allocations = 0;
}

// Introspection
inline bool ArenaAllocator::owns(const void* ptr) const noexcept {
	const auto* p = static_cast<const std::byte*>(ptr);
	return p >= memory && p < memory + cap;
}

inline const ArenaAllocator::Stats& ArenaAllocator::getStats() const noexcept {
	return stats;
}

inline std::size_t ArenaAllocator::capacity() const noexcept {
	return cap;
}

inline std::size_t ArenaAllocator::used() const noexcept {
	return offset;
}

inline std::size_t ArenaAllocator::remaining() const noexcept {
	return cap - offset;
}
