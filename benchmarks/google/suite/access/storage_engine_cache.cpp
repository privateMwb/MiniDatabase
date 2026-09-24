// StorageEngine Cache Benchmark Suite
// Measures page cache access costs: a cache hit (in-memory) vs a cache
// miss (falls through to readPageFromDisk).
//
// Covers:
// - fetchPage cache hit
// - fetchPage cache miss (disk read)
// - getCachedPage (peek-only, never touches disk)

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Measures fetchPage() when the page is already cached.
static void fetch_hit(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "storage_cache_hit";
    fs::remove_all(dir);
    fs::create_directories(dir);

    StorageEngine engine(dir);
    Page p(1);
    (void)p.addRecord(Record(1));
    engine.cachePage("orders", std::move(p));

    for (auto _ : state) {
        Page* out = engine.fetchPage("orders", 1);
        benchmark::DoNotOptimize(out);
    }

    fs::remove_all(dir);
}
BENCHMARK(fetch_hit);

// Measures fetchPage() when the page isn't cached and must be read from
// disk. Re-caches after the first call, so the page is evicted before
// every timed iteration to keep measuring the miss path, not a hit.
static void fetch_miss(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "storage_cache_miss";
    fs::remove_all(dir);
    fs::create_directories(dir);

    StorageEngine engine(dir);
    Page p(1);
    (void)p.addRecord(Record(1));
    (void)engine.writePage("orders", p);

    for (auto _ : state) {
        Page* out = engine.fetchPage("orders", 1);
        benchmark::DoNotOptimize(out);
        (void)engine.evictPage("orders", 1);
    }

    fs::remove_all(dir);
}
BENCHMARK(fetch_miss);

// Measures getCachedPage() -- a peek that never touches disk, even on a
// miss.
static void peek_cache(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "storage_cache_peek";
    fs::remove_all(dir);
    fs::create_directories(dir);

    StorageEngine engine(dir);
    Page p(1);
    (void)p.addRecord(Record(1));
    engine.cachePage("orders", std::move(p));

    for (auto _ : state) {
        Page* out = engine.getCachedPage("orders", 1);
        benchmark::DoNotOptimize(out);
    }

    fs::remove_all(dir);
}
BENCHMARK(peek_cache);