// Database Scaling Benchmark Suite
// Measures how Database::createTable() and Database::getTable() cost
// change as the table count grows -- both are index/hashmap-based
// (O(1)), so this is the direct contrast to Table::insertRecord()'s
// linear-scan growth in TableScalingBenchmark.cpp.
//
// Covers:
// - createTable at 10, 100, 200 existing tables
// - getTable at the same three scales

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Builds a database pre-seeded with `tableCount` empty tables.
static Database makeSeeded(int tableCount) {
    Database db("shop");
    for (int t = 0; t < tableCount; ++t) {
        (void)db.createTable("t" + std::to_string(t), Vector<ColumnDef>{});
    }
    return db;
}

// Measures createTable() against a database that already has 10 tables.
// Drops the new table after each call so the count -- and therefore the
// cost being measured -- stays stable across iterations.
static void db_create_10(benchmark::State& state) {
    Database db = makeSeeded(10);

    for (auto _ : state) {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    }
}
BENCHMARK(db_create_10);

// Measures createTable() against a database that already has 100 tables.
static void db_create_100(benchmark::State& state) {
    Database db = makeSeeded(100);

    for (auto _ : state) {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    }
}
BENCHMARK(db_create_100);

// Measures createTable() against a database that already has 200 tables
// (close to DBConstants::MAX_TABLES == 256).
static void db_create_200(benchmark::State& state) {
    Database db = makeSeeded(200);

    for (auto _ : state) {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    }
}
BENCHMARK(db_create_200);

// Measures getTable() against a database with 10 tables, for direct
// contrast against the create benchmarks above -- the hashmap-based
// index makes this flat regardless of table count.
static void db_get_10(benchmark::State& state) {
    Database db = makeSeeded(10);

    for (auto _ : state) {
        Table* t = db.getTable("t5");
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(db_get_10);

// Measures getTable() against a database with 100 tables.
static void db_get_100(benchmark::State& state) {
    Database db = makeSeeded(100);

    for (auto _ : state) {
        Table* t = db.getTable("t50");
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(db_get_100);

// Measures getTable() against a database with 200 tables.
static void db_get_200(benchmark::State& state) {
    Database db = makeSeeded(200);

    for (auto _ : state) {
        Table* t = db.getTable("t150");
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(db_get_200);