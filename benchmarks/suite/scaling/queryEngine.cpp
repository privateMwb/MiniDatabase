// QueryEngine Scaling Benchmark Suite
// Measures how select()'s scan-with-predicate cost and sort cost change
// as the table grows from 1 page to 1000 pages -- both are expected to
// grow with n (a full scan, and an n log n sort), unlike the O(1)
// index-based lookups benchmarked elsewhere.
//
// Covers:
// - select with a predicate at 1, 100, and 1000 pages
// - select with sort at the same three scales

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Builds a table of `pageCount` full pages, each record carrying an "n"
// INT field equal to its id.
static Table makeSeeded(std::size_t pageCount) {
    Vector<ColumnDef> schema{ColumnDef{"n", ColumnType::INT, false}};
    Table t("bench", 1, schema);

    RecordID n = static_cast<RecordID>(pageCount) * DBConstants::MAX_RECORDS_PAGE;
    for (RecordID i = 0; i < n; ++i) {
        Record r(i);
        (void)r.setField("n", Json(static_cast<int>(i)));
        (void)t.insertRecord(r);
    }
    return t;
}

// Measures select() with a single equality predicate against tables of
// 1, 100, and 1000 pages.
static void bench_select_eq_at_scale() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    Table small = makeSeeded(1);
    Table medium = makeSeeded(100);
    Table large = makeSeeded(1000);

    FilterPredicate pred{"n", Op::EQ, Json(0)};
    std::span<const FilterPredicate> preds(&pred, 1);

    auto selSmall = [&] {
        QueryResult r = engine.select(small, preds);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select eq (1 page)", selSmall);
    std::cout << "\n";

    auto selMedium = [&] {
        QueryResult r = engine.select(medium, preds);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select eq (100 pages)", selMedium);
    std::cout << "\n";

    auto selLarge = [&] {
        QueryResult r = engine.select(large, preds);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select eq (1000 pages)", selLarge);
}

// Measures select() with a sort applied, against the same three scales.
static void bench_select_sort_at_scale() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    Table small = makeSeeded(1);
    Table medium = makeSeeded(100);
    Table large = makeSeeded(1000);

    SortCondition sort{"n", SortOrder::DESC};

    auto sortSmall = [&] {
        QueryResult r = engine.select(small, std::span<const FilterPredicate>{}, &sort);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select sorted (1 page)", sortSmall);
    std::cout << "\n";

    auto sortMedium = [&] {
        QueryResult r = engine.select(medium, std::span<const FilterPredicate>{}, &sort);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select sorted (100 pages)", sortMedium);
    std::cout << "\n";

    auto sortLarge = [&] {
        QueryResult r = engine.select(large, std::span<const FilterPredicate>{}, &sort);
        doNotOptimize(r);
    };
    BENCH_CUSTOM("QueryEngine select sorted (1000 pages)", sortLarge);
}

// Executes all QueryEngine scaling benchmark cases.
static void run_benchmarks() {
    bench_select_eq_at_scale();
    std::cout << "\n";

    bench_select_sort_at_scale();
}

REGISTER_BENCH_SUITE();