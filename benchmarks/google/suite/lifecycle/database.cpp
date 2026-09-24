// Database Construction Benchmark Suite
// Measures Database's constructor cost and move construction empty vs
// populated with many tables -- Database is move-only.
//
// Covers:
// - constructor
// - move construction, empty database
// - move construction, populated database (many tables)

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures the constructor.
static void db_ctor(benchmark::State& state) {
    for (auto _ : state) {
        Database db("shop");
        benchmark::DoNotOptimize(db);
    }
}
BENCHMARK(db_ctor);

// Measures move construction of an empty database.
static void db_move_empty(benchmark::State& state) {
    for (auto _ : state) {
        Database source("shop");
        Database db(std::move(source));
        benchmark::DoNotOptimize(db);
    }
}
BENCHMARK(db_move_empty);

// Measures move construction of a database holding many tables -- shows
// whether move cost scales with table count or stays flat (the
// containers holding Table* are moved, not the tables themselves).
static void db_move_populated(benchmark::State& state) {
    for (auto _ : state) {
        Database source("shop");
        for (int t = 0; t < 50; ++t) {
            (void)source.createTable("t" + std::to_string(t), Vector<ColumnDef>{});
        }
        Database db(std::move(source));
        benchmark::DoNotOptimize(db);
    }
}
BENCHMARK(db_move_populated);