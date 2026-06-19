// Storage Benchmark Suite
// Measures performance of StorageEngine's page caching layer:
//
// - cached page access vs uncached page access (simulated disk read)
// - cache hit rate under repeated access patterns
// - cache behavior under eviction pressure (working set larger than cache)
//
// This benchmark exercises the real LRUCache integration inside
// StorageEngine, using its own built-in hit/miss counters rather
// than manually tracked statistics.

#include <iostream>
#include <chrono>
#include <thread>

#include "../include/Core/Page.h"
#include "../include/Engine/StorageEngine.h"
#include "../libs/VectorPro/VectorPro.h"

#include "utils/BenchTable.h"

// returns elapsed microseconds for a callable
template<typename F>
auto duration(F func) {
    auto start = std::chrono::steady_clock::now();
    func();
    auto end   = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

// prevents the compiler from eliminating unused operations
template<typename T>
inline void doNotOptimize(const T& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(value) : "memory");
#else
    volatile const T* p = &value;
    (void)p;
#endif
}

// Simulates a disk read by reconstructing a page via serialize/deserialize,
// with a small artificial delay standing in for real disk latency.
static Page simulateDiskRead(PageID id) {
    std::this_thread::sleep_for(std::chrono::microseconds(200));

    Page page(id);
    Json data(Json::ObjectType{});
    data["name"]   = "Name" + std::to_string(id);
    data["salary"] = 50000.0;
    (void)page.addRecord(Record(id, data));

    return page;
}

// Cached vs Uncached Access
// measures access time for a page already in the cache vs a simulated disk read
void cachedVsUncachedBenchmark() {
    StorageEngine storage(".");

    Page warm(1);
    Json data(Json::ObjectType{});
    data["name"]   = "Cached";
    data["salary"] = 1000.0;
    (void)warm.addRecord(Record(1, data));
    storage.cachePage(1, std::move(warm));

    std::size_t iterations = 1'000;

    auto cachedTime = duration([&] {
        for (std::size_t i = 0; i < iterations; ++i) {
            Page* p = storage.getCachedPage(1);
            doNotOptimize(p);
        }
    });

    auto uncachedTime = duration([&] {
        for (std::size_t i = 0; i < iterations; ++i) {
            Page p = simulateDiskRead(static_cast<PageID>(1000 + i));
            doNotOptimize(p.getID());
        }
    });

    VectorPro<std::string> labels;
    labels.push_back("Cached (hit)");
    labels.push_back("Uncached (simulated disk)");

    VectorPro<long> times;
    times.push_back(cachedTime.count());
    times.push_back(uncachedTime.count());

    VectorPro<VectorPro<std::string>> tableData;
    tableData.push_back(labels);
    tableData.push_back(benchtable::convert(times, "us"));

    VectorPro<std::string> headers;
    headers.push_back("Access Type");
    headers.push_back("Time (1000 ops)");

    benchtable::table("Cached vs Uncached Page Access", headers, tableData, 56);
}

// Cache Hit Rate Under Repeated Access
// measures the cache's own reported hit rate when repeatedly accessing a small working set
void cacheHitRateBenchmark() {
    StorageEngine storage(".");

    // Fill the cache with a small working set that fits entirely within capacity
    std::size_t workingSetSize = 10;
    for (std::size_t i = 0; i < workingSetSize; ++i) {
        Page page(static_cast<PageID>(i));
        Json data(Json::ObjectType{});
        data["name"]   = "Page" + std::to_string(i);
        data["salary"] = 100.0;
        (void)page.addRecord(Record(i, data));
        storage.cachePage(static_cast<PageID>(i), std::move(page));
    }

    // Repeatedly access the same working set — should be almost all hits
    std::size_t accesses = 1'000;
    for (std::size_t i = 0; i < accesses; ++i) {
        PageID id = static_cast<PageID>(i % workingSetSize);
        Page*  p  = storage.getCachedPage(id);
        doNotOptimize(p);
    }

    VectorPro<std::string> labels;
    labels.push_back("Hits");
    labels.push_back("Misses");
    labels.push_back("Hit Rate");

    VectorPro<std::string> values;
    values.push_back(std::to_string(storage.cacheHits()));
    values.push_back(std::to_string(storage.cacheMisses()));
    values.push_back(std::to_string(storage.cacheHitRate()) + " %");

    VectorPro<VectorPro<std::string>> data;
    data.push_back(labels);
    data.push_back(values);

    VectorPro<std::string> headers;
    headers.push_back("Metric");
    headers.push_back("Value");

    benchtable::table("Cache Hit Rate (working set fits in cache)", headers, data, 56);
}

// Eviction Pressure
// measures cache hit rate when the working set exceeds cache capacity
void evictionPressureBenchmark() {
    StorageEngine storage(".");

    // Working set larger than LRU_CACHE_CAP forces evictions
    std::size_t workingSetSize = DBConstants::LRU_CACHE_CAP * 2;

    std::size_t accesses = 1'000;
    for (std::size_t i = 0; i < accesses; ++i) {
        PageID id = static_cast<PageID>(i % workingSetSize);

        if (!storage.isCached(id)) {
            Page page(id);
            Json data(Json::ObjectType{});
            data["name"]   = "Page" + std::to_string(id);
            data["salary"] = 100.0;
            (void)page.addRecord(Record(id, data));
            storage.cachePage(id, std::move(page));
        }

        Page* p = storage.getCachedPage(id);
        doNotOptimize(p);
    }

    VectorPro<std::string> labels;
    labels.push_back("Hits");
    labels.push_back("Misses");
    labels.push_back("Hit Rate");

    VectorPro<std::string> values;
    values.push_back(std::to_string(storage.cacheHits()));
    values.push_back(std::to_string(storage.cacheMisses()));
    values.push_back(std::to_string(storage.cacheHitRate()) + " %");

    VectorPro<VectorPro<std::string>> data;
    data.push_back(labels);
    data.push_back(values);

    VectorPro<std::string> headers;
    headers.push_back("Metric");
    headers.push_back("Value");

    benchtable::table("Cache Hit Rate (working set exceeds capacity)", headers, data, 56);
}

// Entry Point
int run_storage_benchmarks() {
    cachedVsUncachedBenchmark();
    cacheHitRateBenchmark();
    evictionPressureBenchmark();

    return 0;
}