#pragma once

#include <cstddef>
#include <new>
#include <utility>

class PoolAllocator {
public:
    // Debug Statistics
    struct Stats {
        std::size_t totalAllocated = 0;  
        std::size_t peakUsed       = 0;  
        std::size_t allocations    = 0;  
        std::size_t deallocations  = 0;  
    };

private:
    // Free List Node
    struct FreeNode {
        FreeNode* next;
    };

    // Core Memory
    std::byte* memory;

    // Pool Configuration
    std::size_t blockSize;   
    std::size_t stride;      
    std::size_t blockCount;  
    std::size_t alignment;   

    // Usage Tracking
    std::size_t freeBlockCount;

    // Free List
    FreeNode* freeList;

    // Debug Stats
    Stats stats;

    // Alignment Utilities

    [[nodiscard]] static constexpr std::size_t alignForward(std::size_t value, std::size_t alignment) noexcept;
    [[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t alignment) noexcept;

public:
    // Constructors & Destructor
    explicit PoolAllocator(std::size_t blockSize,
                           std::size_t blockCount,
                           std::size_t alignment = alignof(std::max_align_t));
    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    PoolAllocator(PoolAllocator&& other) noexcept;
    PoolAllocator& operator=(PoolAllocator&& other) noexcept;

    // Core Allocation
    [[nodiscard]] void* allocate() noexcept;
    void deallocate(void* ptr) noexcept;

    // Object Lifecycle
    template<typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args);

    template<typename T>
    void destroy(T* ptr) noexcept;

    // Introspection
    [[nodiscard]] bool owns(const void* ptr) const noexcept;

    [[nodiscard]] const Stats& getStats() const noexcept;

    [[nodiscard]] std::size_t capacity()    const noexcept;
    [[nodiscard]] std::size_t usedBlocks()  const noexcept;
    [[nodiscard]] std::size_t freeBlocks()  const noexcept;
    [[nodiscard]] std::size_t totalBlocks() const noexcept;
    [[nodiscard]] std::size_t blockStride() const noexcept;
};

#include "PoolAllocator.tpp"

