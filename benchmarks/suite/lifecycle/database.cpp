// Database Construction Benchmark Suite
// Measures Database's constructor cost and move construction empty vs
// populated with many tables -- Database is move-only.
//
// Covers:
// - constructor
// - move construction, empty database
// - move construction, populated database (many tables)

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures the constructor.
static void bench_ctor() {
    auto ctor = [] {
        Database db("shop");
        doNotOptimize(db);
    };
    BENCH("Database ctor", ctor);
}

// Measures move construction of an empty database.
static void bench_move_empty() {
    auto move = [] {
        Database source("shop");
        Database db(std::move(source));
        doNotOptimize(db);
    };
    BENCH("Database move ctor (empty)", move);
}

// Measures move construction of a database holding many tables -- shows
// whether move cost scales with table count or stays flat (the
// containers holding Table* are moved, not the tables themselves).
static void bench_move_populated() {
    auto move = [] {
        Database source("shop");
        for (int t = 0; t < 50; ++t) {
            (void)source.createTable("t" + std::to_string(t), Vector<ColumnDef>{});
        }
        Database db(std::move(source));
        doNotOptimize(db);
    };
    BENCH("Database move ctor (50 tables)", move);
}

// Executes all Database construction benchmark cases.
static void run_benchmarks() {
    bench_ctor();
    std::cout << "\n";

    bench_move_empty();
    std::cout << "\n";

    bench_move_populated();
}

REGISTER_BENCH_SUITE();