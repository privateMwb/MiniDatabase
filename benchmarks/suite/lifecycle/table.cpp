// Table Construction Benchmark Suite
// Measures Table's constructor cost as schema size grows, plus move
// construction empty vs populated -- Table is move-only.
//
// Covers:
// - constructor, empty schema
// - constructor, wide schema
// - move construction, empty table
// - move construction, populated table

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures construction with no columns.
static void bench_ctor_empty_schema() {
    auto ctor = [] {
        Table t("orders", 1, Vector<ColumnDef>{});
        doNotOptimize(t);
    };
    BENCH("Table ctor (empty schema)", ctor);
}

// Measures construction with a wide schema (20 columns).
static void bench_ctor_wide_schema() {
    Vector<ColumnDef> schema;
    for (int i = 0; i < 20; ++i) {
        schema.push_back(ColumnDef{"col" + std::to_string(i), ColumnType::INT, false});
    }

    auto ctor = [&] {
        Table t("orders", 1, schema);
        doNotOptimize(t);
    };
    BENCH("Table ctor (20-column schema)", ctor);
}

// Measures move construction of an empty table.
static void bench_move_empty() {
    auto move = [] {
        Table source("orders", 1, Vector<ColumnDef>{});
        Table t(std::move(source));
        doNotOptimize(t);
    };
    BENCH("Table move ctor (empty)", move);
}

// Measures move construction of a populated, multi-page table -- shows
// whether move cost scales with record count or stays flat.
static void bench_move_populated() {
    auto move = [] {
        Table source("orders", 1, Vector<ColumnDef>{});
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
            (void)source.insertRecord(Record(i));
        }
        Table t(std::move(source));
        doNotOptimize(t);
    };
    BENCH_CUSTOM("Table move ctor (populated)", move);
}

// Executes all Table construction benchmark cases.
static void run_benchmarks() {
    bench_ctor_empty_schema();
    std::cout << "\n";

    bench_ctor_wide_schema();
    std::cout << "\n";

    bench_move_empty();
    std::cout << "\n";

    bench_move_populated();
}

REGISTER_BENCH_SUITE();