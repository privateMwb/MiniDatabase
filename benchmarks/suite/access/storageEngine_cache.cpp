// StorageEngine Cache Benchmark Suite
// Measures page cache access costs: a cache hit (in-memory) vs a cache
// miss (falls through to readPageFromDisk).
//
// Covers:
// - fetchPage cache hit
// - fetchPage cache miss (disk read)
// - getCachedPage (peek-only, never touches disk)

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Measures fetchPage() when the page is already cached.
static void bench_fetch_hit() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_storage_cache_hit";
    fs::remove_all(dir);
    fs::create_directories(dir);

    StorageEngine engine(dir);
    Page p(1);
    (void)p.addRecord(Record(1));
    engine.cachePage("orders", std::move(p));

    auto hit = [&] {
        Page* out = engine.fetchPage("orders", 1);
        doNotOptimize(out);
    };
    BENCH("StorageEngine fetchPage (hit)", hit);

    fs::remove_all(dir);
}

// Measures fetchPage() when the page isn't cached and must be read from
// disk. Re-caches after the first call, so the page is evicted before
// every timed iteration to keep measuring the miss path, not a hit.
static void bench_fetch_miss() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_storage_cache_miss";
    fs::remove_all(dir);
    fs::create_directories(dir);

    StorageEngine engine(dir);
    Page p(1);
    (void)p.addRecord(Record(1));
    (void)engine.writePage("orders", p);

    auto miss = [&] {
        Page* out = engine.fetchPage("orders", 1);
        doNotOptimize(out);
        (void)engine.evictPage("orders", 1);
    };
    BENCH("StorageEngine fetchPage (miss)", miss);

    fs::remove_all(dir);
}

// Measures getCachedPage() -- a peek that never touches disk, even on a
// miss.
static void bench_peek_cache() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_storage_cache_peek";
    fs::remove_all(dir);
    fs::create_directories(dir);

    StorageEngine engine(dir);
    Page p(1);
    (void)p.addRecord(Record(1));
    engine.cachePage("orders", std::move(p));

    auto peek = [&] {
        Page* out = engine.getCachedPage("orders", 1);
        doNotOptimize(out);
    };
    BENCH("StorageEngine getCachedPage", peek);

    fs::remove_all(dir);
}

// Executes all StorageEngine cache benchmark cases.
static void run_benchmarks() {
    bench_fetch_hit();
    std::cout << "\n";

    bench_fetch_miss();
    std::cout << "\n";

    bench_peek_cache();
}

REGISTER_BENCH_SUITE();