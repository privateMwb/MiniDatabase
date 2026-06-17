#pragma once

#include <cassert>
#include <cstddef>
#include <new>
#include <utility>
#include <array>

class ArenaAllocator {
public:
    // Debug Statistics
    struct Stats {
        std::size_t totalAllocated = 0;
        std::size_t currentUsed    = 0;
        std::size_t peakUsed       = 0;
        std::size_t allocations    = 0;
    };

private:
    // Core Memory
    std::byte* memory;
    std::size_t cap;
    std::size_t offset;

    // Frame Stack
    static constexpr std::size_t kMaxFrameDepth = 8;
    std::array<std::size_t, kMaxFrameDepth> frameStack;
    std::size_t frameDepth;

    // Debug Stats
    Stats stats;

    // Alignment Utilities
    [[nodiscard]] static constexpr std::size_t alignForward(std::size_t value, std::size_t alignment) noexcept;
    
    [[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t alignment) noexcept;

public:
    // Constructors & Destructor
    explicit ArenaAllocator(std::size_t size);
    ~ArenaAllocator();

    ArenaAllocator(const ArenaAllocator&)            = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ArenaAllocator(ArenaAllocator&& other) noexcept;
    ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;

    // Core Allocation
    [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment) noexcept;

    template<typename T>
    [[nodiscard]] T* allocate() noexcept;

    // Object Lifecycle
    template<typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args);

    template<typename T>
    void destroy(T* ptr) noexcept;

    // Frame Management
    void beginFrame() noexcept;
    void endFrame() noexcept;

    // State Management
    void reset() noexcept;

    // Introspection
    [[nodiscard]] bool owns(const void* ptr) const noexcept;

    [[nodiscard]] const Stats& getStats() const noexcept;

    [[nodiscard]] std::size_t capacity() const noexcept;
    [[nodiscard]] std::size_t used() const noexcept;
    [[nodiscard]] std::size_t remaining() const noexcept;
};

#include "ArenaAllocator.tpp"
