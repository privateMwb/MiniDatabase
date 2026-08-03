// QueryEngine Select Benchmark Suite
// Measures select() cost with predicates and with sorting on a
// multi-page table.
//
// Covers:
// - selectAll (no predicates, no sort)
// - select with a single equality predicate
// - select with sort applied

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Builds a table of n records with an "n" INT field, spanning multiple pages.
static Table makeTable(RecordID n) {
    Vector<ColumnDef> schema{ColumnDef{"n", ColumnType::INT, false}};
    Table t("bench", 1, schema);

    for (RecordID i = 0; i < n; ++i) {
        Record r(i);
        (void)r.setField("n", Json(static_cast<int>(i)));
        (void)t.insertRecord(r);
    }
    return t;
}

// Measures selectAll() over a multi-page table.
static void bench_select_all() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 50);

    auto scan = [&] {
        QueryResult r = engine.selectAll(t);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine selectAll", scan);
}

// Measures select() with a single equality predicate matching one record.
static void bench_select_eq() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 50);

    FilterPredicate pred{"n", Op::EQ, Json(1000)};
    std::span<const FilterPredicate> preds(&pred, 1);

    auto filter = [&] {
        QueryResult r = engine.select(t, preds);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select (eq predicate)", filter);
}

// Measures select() with a sort applied on top of a full scan.
static void bench_select_sort() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 50);

    SortCondition sort{"n", SortOrder::DESC};

    auto sorted = [&] {
        QueryResult r = engine.select(t, std::span<const FilterPredicate>{}, &sort);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select (sorted)", sorted);
}

// Executes all QueryEngine select benchmark cases.
static void run_benchmarks() {
    bench_select_all();
    std::cout << "\n";

    bench_select_eq();
    std::cout << "\n";

    bench_select_sort();
}

REGISTER_BENCH_SUITE();