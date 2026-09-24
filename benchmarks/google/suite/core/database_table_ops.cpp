// Database Table Ops Benchmark Suite
// Measures Database's table-management API: createTable, dropTable,
// getTable, and hasTable, against a database already holding many tables.
//
// Covers:
// - createTable (into an already multi-table database)
// - dropTable
// - getTable (hit)
// - hasTable (hit)

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

constexpr int kSeedTableCount = 50;

// Builds a database with kSeedTableCount tables already created.
static Database makeSeededDatabase() {
    Database db("shop");
    for (int t = 0; t < kSeedTableCount; ++t) {
        (void)db.createTable("t" + std::to_string(t), Vector<ColumnDef>{});
    }
    return db;
}

// Measures createTable() adding a new table into an already multi-table
// database. Drops it again after each call so the table count -- and
// therefore the cost being measured -- stays stable across iterations.
static void db_table_create(benchmark::State& state) {
    Database db = makeSeededDatabase();

    for (auto _ : state) {
        (void)db.createTable("new_table", Vector<ColumnDef>{});
        (void)db.dropTable("new_table");
    }
}
BENCHMARK(db_table_create);

// Measures dropTable() removing an existing table. Re-creates it after
// each call so there's always a table to drop.
static void db_table_drop(benchmark::State& state) {
    Database db = makeSeededDatabase();

    for (auto _ : state) {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    }
}
BENCHMARK(db_table_drop);

// Measures getTable() looking up an existing table by name.
static void db_table_get(benchmark::State& state) {
    Database db = makeSeededDatabase();

    for (auto _ : state) {
        Table* t = db.getTable("t25");
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(db_table_get);

// Measures hasTable() checking for an existing table by name.
static void db_table_has(benchmark::State& state) {
    Database db = makeSeededDatabase();

    for (auto _ : state) {
        bool b = db.hasTable("t25");
        benchmark::DoNotOptimize(b);
    }
}
BENCHMARK(db_table_has);
