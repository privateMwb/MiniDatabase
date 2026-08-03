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

#include <support/framework.h>

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
static void bench_insert_1_page() {
    Table t = makeSeeded(1);
    RecordID nextId = DBConstants::MAX_RECORDS_PAGE;

    auto insert = [&] { (void)t.insertRecord(Record(nextId++)); };
    BENCH_CUSTOM("Table insertRecord (1 page)", insert);
}

// Measures insertRecord() against a table that already has 100 full
// pages -- findPageWithSlot() now scans up to 100 pages before landing on
// the one open slot.
static void bench_insert_100_pages() {
    Table t = makeSeeded(100);
    RecordID nextId = static_cast<RecordID>(100) * DBConstants::MAX_RECORDS_PAGE;

    auto insert = [&] { (void)t.insertRecord(Record(nextId++)); };
    BENCH_CUSTOM("Table insertRecord (100 pages)", insert);
}

// Measures insertRecord() against a table that already has 1000 full
// pages. A small iteration count keeps the table's size roughly stable
// across the run, so the measurement stays representative of "cost at
// ~1000 pages" rather than drifting upward as the loop itself grows the
// table.
static void bench_insert_1000_pages() {
    Table t = makeSeeded(1000);
    RecordID nextId = static_cast<RecordID>(1000) * DBConstants::MAX_RECORDS_PAGE;

    auto insert = [&] { (void)t.insertRecord(Record(nextId++)); };
    BENCH_CUSTOM("Table insertRecord (1000 pages)", insert);
}

// Measures getRecord() at the same three scales, for direct contrast --
// the index makes this flat regardless of page count.
static void bench_get_at_scale() {
    Table small = makeSeeded(1);
    Table medium = makeSeeded(100);
    Table large = makeSeeded(1000);
    Record out;

    auto getSmall = [&] {
        (void)small.getRecord(0, out);
        doNotOptimize(out);
    };
    BENCH("Table getRecord (1 page)", getSmall);
    std::cout << "\n";

    auto getMedium = [&] {
        (void)medium.getRecord(0, out);
        doNotOptimize(out);
    };
    BENCH("Table getRecord (100 pages)", getMedium);
    std::cout << "\n";

    auto getLarge = [&] {
        (void)large.getRecord(0, out);
        doNotOptimize(out);
    };
    BENCH("Table getRecord (1000 pages)", getLarge);
}

// Executes all Table scaling benchmark cases.
static void run_benchmarks() {
    bench_insert_1_page();
    std::cout << "\n";

    bench_insert_100_pages();
    std::cout << "\n";

    bench_insert_1000_pages();
    std::cout << "\n";

    bench_get_at_scale();
}

REGISTER_BENCH_SUITE();