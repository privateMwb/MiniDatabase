// QueryEngine Scaling Benchmark Suite
// Measures how select()'s scan-with-predicate cost and sort cost change
// as the table grows from 1 page to 1000 pages -- both are expected to
// grow with n (a full scan, and an n log n sort), unlike the O(1)
// index-based lookups benchmarked elsewhere.
//
// Covers:
// - select with a predicate at 1, 100, and 1000 pages
// - select with sort at the same three scales

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

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

// Measures select() with a single equality predicate against a 1-page
// table.
static void select_eq_pages_1(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeSeeded(1);

    FilterPredicate pred{"n", Op::EQ, Json(0)};
    std::span<const FilterPredicate> preds(&pred, 1);

    for (auto _ : state) {
        QueryResult r = engine.select(t, preds);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(select_eq_pages_1);

// Measures select() with a single equality predicate against a 100-page
// table.
static void select_eq_pages_100(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeSeeded(100);

    FilterPredicate pred{"n", Op::EQ, Json(0)};
    std::span<const FilterPredicate> preds(&pred, 1);

    for (auto _ : state) {
        QueryResult r = engine.select(t, preds);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(select_eq_pages_100);

// Measures select() with a single equality predicate against a
// 1000-page table.
static void select_eq_pages_1000(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeSeeded(1000);

    FilterPredicate pred{"n", Op::EQ, Json(0)};
    std::span<const FilterPredicate> preds(&pred, 1);

    for (auto _ : state) {
        QueryResult r = engine.select(t, preds);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(select_eq_pages_1000);

// Measures select() with a sort applied against a 1-page table.
static void select_sort_pages_1(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeSeeded(1);

    SortCondition sort{"n", SortOrder::DESC};

    for (auto _ : state) {
        QueryResult r = engine.select(t, std::span<const FilterPredicate>{}, &sort);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(select_sort_pages_1);

// Measures select() with a sort applied against a 100-page table.
static void select_sort_pages_100(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeSeeded(100);

    SortCondition sort{"n", SortOrder::DESC};

    for (auto _ : state) {
        QueryResult r = engine.select(t, std::span<const FilterPredicate>{}, &sort);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(select_sort_pages_100);

// Measures select() with a sort applied against a 1000-page table.
static void select_sort_pages_1000(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeSeeded(1000);

    SortCondition sort{"n", SortOrder::DESC};

    for (auto _ : state) {
        QueryResult r = engine.select(t, std::span<const FilterPredicate>{}, &sort);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(select_sort_pages_1000);