// QueryEngine Select Benchmark Suite
// Measures select() cost with predicates and with sorting on a
// multi-page table.
//
// Covers:
// - selectAll (no predicates, no sort)
// - select with a single equality predicate
// - select with sort applied

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

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
static void qe_select_all(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 50);

    for (auto _ : state) {
        QueryResult r = engine.selectAll(t);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(qe_select_all);

// Measures select() with a single equality predicate matching one record.
static void qe_select_eq(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 50);

    FilterPredicate pred{"n", Op::EQ, Json(1000)};
    std::span<const FilterPredicate> preds(&pred, 1);

    for (auto _ : state) {
        QueryResult r = engine.select(t, preds);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(qe_select_eq);

// Measures select() with a sort applied on top of a full scan.
static void qe_select_sort(benchmark::State& state) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 50);

    SortCondition sort{"n", SortOrder::DESC};

    for (auto _ : state) {
        QueryResult r = engine.select(t, std::span<const FilterPredicate>{}, &sort);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(qe_select_sort);
