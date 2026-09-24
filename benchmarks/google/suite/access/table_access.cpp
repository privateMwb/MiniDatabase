// Table Access Benchmark Suite
// Measures Table::getRecord() through the RecordID -> PageID index,
// contrasting a single-page table against a multi-page table to show the
// index keeps lookup cost flat as the table grows (O(1) hash lookup +
// O(1) page lookup, not a scan over pages).
//
// Covers:
// - getRecord hit, single-page table
// - getRecord hit, multi-page table
// - getRecord miss

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Builds a table with n records (ids 0..n-1), spanning however many pages
// that requires.
static Table makeTable(RecordID n) {
    Table t("bench", 1, Vector<ColumnDef>{});
    for (RecordID i = 0; i < n; ++i) {
        (void)t.insertRecord(Record(i));
    }
    return t;
}

// Measures getRecord() on a table small enough to fit in one page.
static void getRecord_hit_small(benchmark::State& state) {
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE / 2);
    RecordID id = DBConstants::MAX_RECORDS_PAGE / 4;
    Record out;

    for (auto _ : state) {
        (void)t.getRecord(id, out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(getRecord_hit_small);

// Measures getRecord() on a table spanning many pages -- same cost as the
// small case is the point being measured.
static void getRecord_hit_large(benchmark::State& state) {
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 200);
    RecordID id = DBConstants::MAX_RECORDS_PAGE * 150;
    Record out;

    for (auto _ : state) {
        (void)t.getRecord(id, out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(getRecord_hit_large);

// Measures getRecord() for an id that was never inserted.
static void getRecord_miss(benchmark::State& state) {
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 200);
    Record out;

    for (auto _ : state) {
        (void)t.getRecord(999'999'999, out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(getRecord_miss);
