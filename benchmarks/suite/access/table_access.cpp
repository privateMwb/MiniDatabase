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

#include <support/framework.h>

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
static void bench_get_hit_small() {
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE / 2);
    RecordID id = DBConstants::MAX_RECORDS_PAGE / 4;
    Record out;

    auto hit = [&] {
        (void)t.getRecord(id, out);
        doNotOptimize(out);
    };
    BENCH("Table getRecord (single page)", hit);
}

// Measures getRecord() on a table spanning many pages -- same cost as the
// small case is the point being measured.
static void bench_get_hit_large() {
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 200);
    RecordID id = DBConstants::MAX_RECORDS_PAGE * 150;
    Record out;

    auto hit = [&] {
        (void)t.getRecord(id, out);
        doNotOptimize(out);
    };
    BENCH("Table getRecord (multi page)", hit);
}

// Measures getRecord() for an id that was never inserted.
static void bench_get_miss() {
    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 200);
    Record out;

    auto miss = [&] {
        (void)t.getRecord(999'999'999, out);
        doNotOptimize(out);
    };
    BENCH("Table getRecord (miss)", miss);
}

// Executes all Table access benchmark cases.
static void run_benchmarks() {
    bench_get_hit_small();
    std::cout << "\n";

    bench_get_hit_large();
    std::cout << "\n";

    bench_get_miss();
}

REGISTER_BENCH_SUITE();