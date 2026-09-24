// Table Scaling Benchmark Suite
// Measures how Table::insertRecord() and Table::getRecord() cost change
// as the table grows from a handful of pages to many hundreds.
// getRecord() is index-based (O(1), flat regardless of size), but
// insertRecord()'s findPageWithSlot() linearly scans every existing page
// looking for a non-full one before allocating a new page -- so its cost
// is expected to grow with page count, not stay flat like getRecord().
//
// Covers:
// - insertRecord at 1 page, 100 pages, 1000 pages of existing data
// - getRecord at the same three scales, for contrast

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Builds a table pre-seeded with `pageCount` full pages of records.
static Table makeSeeded(std::size_t pageCount) {
    Table t("orders", 1, Vector<ColumnDef>{});
    RecordID n = static_cast<RecordID>(pageCount) * DBConstants::MAX_RECORDS_PAGE;
    for (RecordID i = 0; i < n; ++i) {
        (void)t.insertRecord(Record(i));
    }
    return t;
}

// Measures insertRecord() against a table that already has 1 full page.
static void insertRecord_pages_1(benchmark::State& state) {
    Table t = makeSeeded(1);
    RecordID nextId = DBConstants::MAX_RECORDS_PAGE;

    for (auto _ : state) {
        (void)t.insertRecord(Record(nextId++));
    }
}
BENCHMARK(insertRecord_pages_1);

// Measures insertRecord() against a table that already has 100 full
// pages -- findPageWithSlot() now scans up to 100 pages before landing on
// the one open slot.
static void insertRecord_pages_100(benchmark::State& state) {
    Table t = makeSeeded(100);
    RecordID nextId = static_cast<RecordID>(100) * DBConstants::MAX_RECORDS_PAGE;

    for (auto _ : state) {
        (void)t.insertRecord(Record(nextId++));
    }
}
BENCHMARK(insertRecord_pages_100);

// Measures insertRecord() against a table that already has 1000 full
// pages.
static void insertRecord_pages_1000(benchmark::State& state) {
    Table t = makeSeeded(1000);
    RecordID nextId = static_cast<RecordID>(1000) * DBConstants::MAX_RECORDS_PAGE;

    for (auto _ : state) {
        (void)t.insertRecord(Record(nextId++));
    }
}
BENCHMARK(insertRecord_pages_1000);

// Measures getRecord() against a 1-page table, for direct contrast
// against the insert benchmarks above -- the index makes this flat
// regardless of page count.
static void getRecord_pages_1(benchmark::State& state) {
    Table t = makeSeeded(1);
    Record out;

    for (auto _ : state) {
        (void)t.getRecord(0, out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(getRecord_pages_1);

// Measures getRecord() against a 100-page table.
static void getRecord_pages_100(benchmark::State& state) {
    Table t = makeSeeded(100);
    Record out;

    for (auto _ : state) {
        (void)t.getRecord(0, out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(getRecord_pages_100);

// Measures getRecord() against a 1000-page table.
static void getRecord_pages_1000(benchmark::State& state) {
    Table t = makeSeeded(1000);
    Record out;

    for (auto _ : state) {
        (void)t.getRecord(0, out);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(getRecord_pages_1000);