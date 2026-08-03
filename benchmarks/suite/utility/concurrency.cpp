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

#include <support/framework.h>

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
static void bench_save_all() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_concurrency_save";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string base = dir + "/db";

    Database db = makeDatabase();
    Concurrency conc;

    auto save = [&] { (void)conc.saveAllTablesParallel(db, base); };
    BENCH_CUSTOM("Concurrency saveAllTablesParallel", save);

    fs::remove_all(dir);
}

// Measures loadAllTablesParallel() across all tables. Reloads into a
// fresh set of empty tables each iteration, since importTableFromFile
// rejects duplicate ids.
static void bench_load_all() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_concurrency_load";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string base = dir + "/db";

    Database seed = makeDatabase();
    Concurrency conc;
    (void)conc.saveAllTablesParallel(seed, base);

    auto load = [&] {
        Database db("shop");
        for (int t = 0; t < kTableCount; ++t) {
            (void)db.createTable("t" + std::to_string(t), Vector<ColumnDef>{});
        }
        (void)conc.loadAllTablesParallel(db, base);
        doNotOptimize(db);
    };
    BENCH_CUSTOM("Concurrency loadAllTablesParallel", load);

    fs::remove_all(dir);
}

// Measures rebuildAllIndexesParallel() across all tables.
static void bench_rebuild_all() {
    Database db = makeDatabase();
    Concurrency conc;

    auto rebuild = [&] { (void)conc.rebuildAllIndexesParallel(db); };
    BENCH_CUSTOM("Concurrency rebuildAllIndexesParallel", rebuild);
}

// Measures exportAllTablesParallel() across all tables.
static void bench_export_all() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_concurrency_export";
    fs::remove_all(dir);
    fs::create_directories(dir);

    Database db = makeDatabase();
    Concurrency conc;

    auto exportAll = [&] { (void)conc.exportAllTablesParallel(db, dir); };
    BENCH_CUSTOM("Concurrency exportAllTablesParallel", exportAll);

    fs::remove_all(dir);
}

// Executes all Concurrency benchmark cases.
static void run_benchmarks() {
    bench_save_all();
    std::cout << "\n";

    bench_load_all();
    std::cout << "\n";

    bench_rebuild_all();
    std::cout << "\n";

    bench_export_all();
}

REGISTER_BENCH_SUITE();