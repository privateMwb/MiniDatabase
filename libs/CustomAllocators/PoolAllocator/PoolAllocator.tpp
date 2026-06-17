#include <cassert>

// Alignment Utilities
constexpr bool PoolAllocator::isPowerOfTwo(std::size_t alignment) noexcept {
	return alignment != 0 && (alignment & (alignment - 1)) == 0;
}

constexpr std::size_t PoolAllocator::alignForward(std::size_t value, std::size_t alignment) noexcept {
	return (value + alignment - 1) & ~(alignment - 1);
}

// Constructors & Destructor
inline PoolAllocator::PoolAllocator(std::size_t blockSize,
                                    std::size_t blockCount,
                                    std::size_t alignment)
	: blockSize(blockSize < sizeof(FreeNode) ? sizeof(FreeNode) : blockSize)
	, stride(alignForward(blockSize < sizeof(FreeNode) ? sizeof(FreeNode) : blockSize, alignment))
	, blockCount(blockCount)
	, alignment(alignment)
	, freeBlockCount(blockCount)
	, freeList(nullptr)
	, stats{}
{
	assert(isPowerOfTwo(alignment) && "alignment must be a non-zero power of two");
	assert(blockCount > 0          && "blockCount must be > 0");

	memory   = static_cast<std::byte*>(::operator new(stride * blockCount, std::align_val_t(alignment)));

	freeList = reinterpret_cast<FreeNode*>(memory);

	FreeNode* current = freeList;

	for (std::size_t i = 0; i < blockCount - 1; ++i) {
		current->next = reinterpret_cast<FreeNode*>(
		                    reinterpret_cast<std::byte*>(current) + stride);
		current = current->next;
	}

	current->next = nullptr;
}

inline PoolAllocator::~PoolAllocator() {
	::operator delete(memory, std::align_val_t(alignment));
}

inline PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
	: memory(other.memory)
	, blockSize(other.blockSize)
	, stride(other.stride)
	, blockCount(other.blockCount)
	, alignment(other.alignment)
	, freeBlockCount(other.freeBlockCount)
	, freeList(other.freeList)
	, stats(other.stats)
{
	other.memory         = nullptr;
	other.blockSize      = 0;
	other.stride         = 0;
	other.blockCount     = 0;
	other.alignment      = 0;
	other.freeBlockCount = 0;
	other.freeList       = nullptr;
	other.stats          = {};
}

inline PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
	if (this != &other) {
		::operator delete(memory, std::align_val_t(alignment));

		memory         = other.memory;
		blockSize      = other.blockSize;
		stride         = other.stride;
		blockCount     = other.blockCount;
		alignment      = other.alignment;
		freeBlockCount = other.freeBlockCount;
		freeList       = other.freeList;
		stats          = other.stats;

		other.memory         = nullptr;
		other.blockSize      = 0;
		other.stride         = 0;
		other.blockCount     = 0;
		other.alignment      = 0;
		other.freeBlockCount = 0;
		other.freeList       = nullptr;
		other.stats          = {};
	}

	return *this;
}

// Core Allocation
inline void* PoolAllocator::allocate() noexcept {
	if (freeBlockCount == 0) [[unlikely]]
		return nullptr;

	FreeNode* node = freeList;
	freeList       = freeList->next;

	--freeBlockCount;

	++stats.allocations;
	stats.totalAllocated += stride;

	std::size_t used = blockCount - freeBlockCount;
	if (used > stats.peakUsed) [[unlikely]]
		stats.peakUsed = used;

	return static_cast<void*>(node);
}

inline void PoolAllocator::deallocate(void* ptr) noexcept {
	if (!ptr || !owns(ptr)) [[unlikely]] return;

	FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
	node->next     = freeList;
	freeList       = node;

	++freeBlockCount;
	++stats.deallocations;
}

// Object Lifecycle
template<typename T, typename... Args>
T* PoolAllocator::create(Args&&... args) {
	static_assert(sizeof(T) <= sizeof(std::byte) * 512,
	              "T may be too large for a pool block — verify blockSize");

	if (sizeof(T) > blockSize || alignof(T) > alignment) [[unlikely]]
		return nullptr;

	void* raw = allocate();
	if (!raw) [[unlikely]] return nullptr;

	return ::new (raw) T(std::forward<Args>(args)...);
}

template<typename T>
void PoolAllocator::destroy(T* ptr) noexcept {
	if (!ptr || !owns(ptr)) [[unlikely]] return;

	ptr->~T();
	deallocate(ptr);
}

// Introspection
inline bool PoolAllocator::owns(const void* ptr) const noexcept {
	const auto* p     = static_cast<const std::byte*>(ptr);
	const auto* start = memory;
	const auto* end   = memory + (stride * blockCount);

	return p >= start && p < end;
}

inline const PoolAllocator::Stats& PoolAllocator::getStats() const noexcept {
	return stats;
}

inline std::size_t PoolAllocator::capacity() const noexcept {
	return stride * blockCount;
}

inline std::size_t PoolAllocator::usedBlocks() const noexcept {
	return blockCount - freeBlockCount;
}

inline std::size_t PoolAllocator::freeBlocks() const noexcept {
	return freeBlockCount;
}

inline std::size_t PoolAllocator::totalBlocks() const noexcept {
	return blockCount;
}

inline std::size_t PoolAllocator::blockStride() const noexcept {
	return stride;
}

