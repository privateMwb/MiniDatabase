// Database Scaling Benchmark Suite
// Measures how Database::createTable() and Database::getTable() cost
// change as the table count grows -- both are index/hashmap-based
// (O(1)), so this is the direct contrast to Table::insertRecord()'s
// linear-scan growth in TableScalingBenchmark.cpp.
//
// Covers:
// - createTable at 10, 100, 200 existing tables
// - getTable at the same three scales

#include <support/framework.h>

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
static void bench_create_10() {
    Database db = makeSeeded(10);

    auto create = [&] {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    };
    BENCH("Database createTable (10 tables)", create);
}

// Measures createTable() against a database that already has 100 tables.
static void bench_create_100() {
    Database db = makeSeeded(100);

    auto create = [&] {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    };
    BENCH("Database createTable (100 tables)", create);
}

// Measures createTable() against a database that already has 200 tables
// (close to DBConstants::MAX_TABLES == 256).
static void bench_create_200() {
    Database db = makeSeeded(200);

    auto create = [&] {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    };
    BENCH("Database createTable (200 tables)", create);
}

// Measures getTable() at the same three scales, for direct contrast --
// the hashmap-based index makes this flat regardless of table count.
static void bench_get_at_scale() {
    Database small = makeSeeded(10);
    Database medium = makeSeeded(100);
    Database large = makeSeeded(200);

    auto getSmall = [&] {
        Table* t = small.getTable("t5");
        doNotOptimize(t);
    };
    BENCH("Database getTable (10 tables)", getSmall);
    std::cout << "\n";

    auto getMedium = [&] {
        Table* t = medium.getTable("t50");
        doNotOptimize(t);
    };
    BENCH("Database getTable (100 tables)", getMedium);
    std::cout << "\n";

    auto getLarge = [&] {
        Table* t = large.getTable("t150");
        doNotOptimize(t);
    };
    BENCH("Database getTable (200 tables)", getLarge);
}

// Executes all Database scaling benchmark cases.
static void run_benchmarks() {
    bench_create_10();
    std::cout << "\n";

    bench_create_100();
    std::cout << "\n";

    bench_create_200();
    std::cout << "\n";

    bench_get_at_scale();
}

REGISTER_BENCH_SUITE();