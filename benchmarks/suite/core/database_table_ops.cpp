// Database Table Ops Benchmark Suite
// Measures Database's table-management API: createTable, dropTable,
// getTable, and hasTable, against a database already holding many tables.
//
// Covers:
// - createTable (into an already multi-table database)
// - dropTable
// - getTable (hit)
// - hasTable (hit)

#include <support/framework.h>

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
static void bench_create_table() {
    Database db = makeSeededDatabase();

    auto create = [&] {
        (void)db.createTable("new_table", Vector<ColumnDef>{});
        (void)db.dropTable("new_table");
    };
    BENCH("Database createTable", create);
}

// Measures dropTable() removing an existing table. Re-creates it after
// each call so there's always a table to drop.
static void bench_drop_table() {
    Database db = makeSeededDatabase();

    auto drop = [&] {
        (void)db.createTable("scratch", Vector<ColumnDef>{});
        (void)db.dropTable("scratch");
    };
    BENCH("Database dropTable", drop);
}

// Measures getTable() looking up an existing table by name.
static void bench_get_table() {
    Database db = makeSeededDatabase();

    auto get = [&] {
        Table* t = db.getTable("t25");
        doNotOptimize(t);
    };
    BENCH("Database getTable", get);
}

// Measures hasTable() checking for an existing table by name.
static void bench_has_table() {
    Database db = makeSeededDatabase();

    auto has = [&] {
        bool b = db.hasTable("t25");
        doNotOptimize(b);
    };
    BENCH("Database hasTable", has);
}

// Executes all Database table-ops benchmark cases.
static void run_benchmarks() {
    bench_create_table();
    std::cout << "\n";

    bench_drop_table();
    std::cout << "\n";

    bench_get_table();
    std::cout << "\n";

    bench_has_table();
}

REGISTER_BENCH_SUITE();