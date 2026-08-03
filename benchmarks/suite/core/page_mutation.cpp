// Page Mutation Benchmark Suite
// Measures Page's record-lifecycle mutators: addRecord, updateRecord,
// deleteRecord (soft delete), and compact() (reclaiming deleted slots).
//
// Covers:
// - addRecord
// - updateRecord (existing record)
// - deleteRecord (soft delete)
// - compact (half the page deleted)

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures addRecord() filling a page from empty. Rebuilds the page each
// iteration since a full page rejects further inserts.
static void bench_add_record() {
    auto add = [] {
        Page p(1);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)p.addRecord(Record(i));
        }
        doNotOptimize(p);
    };
    BENCH("Page addRecord (fill page)", add);
}

// Measures updateRecord() replacing an existing record's data.
static void bench_update_record() {
    Page p(1);
    (void)p.addRecord(Record(1));
    Record updated(1);
    (void)updated.setField("v", Json(2));

    auto update = [&] { (void)p.updateRecord(updated); };
    BENCH("Page updateRecord", update);
}

// Measures deleteRecord() soft-deleting a record. Re-adds the record
// before every timed call so there's always something to delete.
static void bench_delete_record() {
    Page p(1);

    auto del = [&] {
        (void)p.addRecord(Record(1));
        (void)p.deleteRecord(1);
    };
    BENCH("Page deleteRecord", del);
}

// Measures compact() reclaiming a page that's half soft-deleted.
static void bench_compact() {
    auto compact = [] {
        Page p(1);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)p.addRecord(Record(i));
        }
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; i += 2) {
            (void)p.deleteRecord(i);
        }
        (void)p.compact();
        doNotOptimize(p);
    };
    BENCH("Page compact (half deleted)", compact);
}

// Executes all Page mutation benchmark cases.
static void run_benchmarks() {
    bench_add_record();
    std::cout << "\n";

    bench_update_record();
    std::cout << "\n";

    bench_delete_record();
    std::cout << "\n";

    bench_compact();
}

REGISTER_BENCH_SUITE();