// Table Construction Benchmark Suite
// Measures Table's constructor cost as schema size grows, plus move
// construction empty vs populated -- Table is move-only.
//
// Covers:
// - constructor, empty schema
// - constructor, wide schema
// - move construction, empty table
// - move construction, populated table

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures construction with no columns.
static void ctor_schema_empty(benchmark::State& state) {
    for (auto _ : state) {
        Table t("orders", 1, Vector<ColumnDef>{});
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(ctor_schema_empty);

// Measures construction with a wide schema (20 columns).
static void ctor_schema_wide(benchmark::State& state) {
    Vector<ColumnDef> schema;
    for (int i = 0; i < 20; ++i) {
        schema.push_back(ColumnDef{"col" + std::to_string(i), ColumnType::INT, false});
    }

    for (auto _ : state) {
        Table t("orders", 1, schema);
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(ctor_schema_wide);

// Measures move construction of an empty table.
static void table_move_empty(benchmark::State& state) {
    for (auto _ : state) {
        Table source("orders", 1, Vector<ColumnDef>{});
        Table t(std::move(source));
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(table_move_empty);

// Measures move construction of a populated, multi-page table -- shows
// whether move cost scales with record count or stays flat.
static void table_move_populated(benchmark::State& state) {
    for (auto _ : state) {
        Table source("orders", 1, Vector<ColumnDef>{});
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
            (void)source.insertRecord(Record(i));
        }
        Table t(std::move(source));
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(table_move_populated);