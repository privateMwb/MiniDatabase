// Concurrency Benchmark Suite
// Measures the parallel batch operations against a fixed multi-table
// database: parallel save/load, parallel index rebuild, and parallel
// export.
//
// Covers:
// - saveAllTablesParallel
// - loadAllTablesParallel
// - rebuildAllIndexesParallel
// - exportAllTablesParallel

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

constexpr int kTableCount = 8;

// Builds a database with kTableCount tables, each holding one full page
// of records.
static Database makeDatabase() {
    Database db("shop");
    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        (void)db.createTable(name, Vector<ColumnDef>{});
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)db.getTable(name)->insertRecord(Record(i));
        }
    }
    return db;
}

// Measures saveAllTablesParallel() across all tables.
static void all_save(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "concurrency_save";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string base = dir + "/db";

    Database db = makeDatabase();
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.saveAllTablesParallel(db, base);
    }

    fs::remove_all(dir);
}
BENCHMARK(all_save);

// Measures loadAllTablesParallel() across all tables. Reloads into a
// fresh set of empty tables each iteration, since importTableFromFile
// rejects duplicate ids.
static void all_load(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "concurrency_load";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string base = dir + "/db";

    Database seed = makeDatabase();
    Concurrency conc;
    (void)conc.saveAllTablesParallel(seed, base);

    for (auto _ : state) {
        Database db("shop");
        for (int t = 0; t < kTableCount; ++t) {
            (void)db.createTable("t" + std::to_string(t), Vector<ColumnDef>{});
        }
        (void)conc.loadAllTablesParallel(db, base);
        benchmark::DoNotOptimize(db);
    }

    fs::remove_all(dir);
}
BENCHMARK(all_load);

// Measures rebuildAllIndexesParallel() across all tables.
static void all_rebuild(benchmark::State& state) {
    Database db = makeDatabase();
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.rebuildAllIndexesParallel(db);
    }
}
BENCHMARK(all_rebuild);

// Measures exportAllTablesParallel() across all tables.
static void all_export(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "concurrency_export";
    fs::remove_all(dir);
    fs::create_directories(dir);

    Database db = makeDatabase();
    Concurrency conc;

    for (auto _ : state) {
        (void)conc.exportAllTablesParallel(db, dir);
    }

    fs::remove_all(dir);
}
BENCHMARK(all_export);