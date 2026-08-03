// Page Access Benchmark Suite
// Measures record lookup performance on a full page: getRecord (id-based,
// linear scan, skips deleted) vs getRecordAt (index-based, direct
// positional access).
//
// Covers:
// - getRecord hit (worst-case position: last slot)
// - getRecord miss (full-page scan, no match)
// - getRecordAt hit (direct index)
// - getRecordAt out-of-range (miss)

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Builds a full page (MAX_RECORDS_PAGE records, ids 0..N-1) for benchmarking.
static Page makeFullPage() {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        (void)p.addRecord(Record(i));
    }
    return p;
}

// Measures getRecord() hitting the last record in a full page -- the
// worst case for a linear scan.
static void bench_get_record_hit() {
    Page p = makeFullPage();
    RecordID lastId = DBConstants::MAX_RECORDS_PAGE - 1;

    auto byId = [&] {
        Record* r = p.getRecord(lastId);
        doNotOptimize(r);
    };
    BENCH("Page getRecord (id, last slot)", byId);
}

// Measures getRecordAt() at the same position -- direct index access, no
// scan required.
static void bench_get_at_hit() {
    Page p = makeFullPage();
    std::size_t lastIndex = DBConstants::MAX_RECORDS_PAGE - 1;

    auto byIndex = [&] {
        Record* r = p.getRecordAt(lastIndex);
        doNotOptimize(r);
    };
    BENCH("Page getRecordAt (index, last slot)", byIndex);
}

// Measures getRecord() scanning an entire full page for an id that doesn't
// exist -- the worst case for a miss.
static void bench_get_record_miss() {
    Page p = makeFullPage();

    auto missById = [&] {
        Record* r = p.getRecord(9999);
        doNotOptimize(r);
    };
    BENCH("Page getRecord (id, miss)", missById);
}

// Measures getRecordAt() past the end of the page -- a bounds check, no
// scan required even on miss.
static void bench_get_at_miss() {
    Page p = makeFullPage();

    auto missByIndex = [&] {
        Record* r = p.getRecordAt(9999);
        doNotOptimize(r);
    };
    BENCH("Page getRecordAt (index, out of range)", missByIndex);
}

// Executes all Page access benchmark cases.
static void run_benchmarks() {
    bench_get_record_hit();
    std::cout << "\n";

    bench_get_at_hit();
    std::cout << "\n";

    bench_get_record_miss();
    std::cout << "\n";

    bench_get_at_miss();
}

REGISTER_BENCH_SUITE();