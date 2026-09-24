// Concurrency Scaling Benchmark Suite
// Measures rebuildAllIndexesParallel() cost as table count grows relative
// to the default thread pool size (DBConstants::THREAD_POOL_SIZE == 4).
// rebuildIndex() is pure in-memory work with no I/O, which isolates
// thread-pool scheduling/collection overhead from disk cost -- unlike the
// save/load/export benchmarks in ConcurrencyBenchmark.cpp.
//
// Covers:
// - below the pool size (2 tables)
// - at the pool size (4 tables)
// - above the pool size (16 tables)
// - well above the pool size (64 tables)

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Builds a database with `tableCount` tables, each holding one full page
// of records (enough real work per table that scheduling overhead isn't
// the only thing being measured).
static Database makeSeeded(int tableCount) {
    Database db("shop");
    for (int t = 0; t < tableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        (void)db.createTable(name, Vector<ColumnDef>{});
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)db.getTable(name)->insertRecord(Record(i));
        }
    }
    return db;
}

// Measures rebuildAllIndexesParallel() with fewer tables than pool
// threads -- some worker threads necessarily sit idle.
static void rebuild_pool_below(benchmark::State& state) {
    Database db = makeSeeded(2);
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.rebuildAllIndexesParallel(db);
    }
}
BENCHMARK(rebuild_pool_below);

// Measures rebuildAllIndexesParallel() with exactly as many tables as
// pool threads -- the ideal one-task-per-thread case.
static void rebuild_pool_at(benchmark::State& state) {
    Database db = makeSeeded(4);
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.rebuildAllIndexesParallel(db);
    }
}
BENCHMARK(rebuild_pool_at);

// Measures rebuildAllIndexesParallel() with more tables than pool
// threads -- some threads must process more than one table.
static void rebuild_pool_above(benchmark::State& state) {
    Database db = makeSeeded(16);
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.rebuildAllIndexesParallel(db);
    }
}
BENCHMARK(rebuild_pool_above);

// Measures rebuildAllIndexesParallel() well above pool capacity, showing
// whether cost scales roughly linearly with (tableCount / poolSize) once
// the ideal 1:1 ratio is left behind.
static void rebuild_pool_wellabove(benchmark::State& state) {
    Database db = makeSeeded(64);
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.rebuildAllIndexesParallel(db);
    }
}
BENCHMARK(rebuild_pool_wellabove);