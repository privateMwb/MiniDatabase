// Page Mutation Benchmark Suite
// Measures Page's record-lifecycle mutators: addRecord, updateRecord,
// deleteRecord (soft delete), and compact() (reclaiming deleted slots).
//
// Covers:
// - addRecord
// - updateRecord (existing record)
// - deleteRecord (soft delete)
// - compact (half the page deleted)

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures addRecord() filling a page from empty. Rebuilds the page each
// iteration since a full page rejects further inserts.
static void record_add(benchmark::State& state) {
    for (auto _ : state) {
        Page p(1);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)p.addRecord(Record(i));
        }
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(record_add);

// Measures updateRecord() replacing an existing record's data.
static void record_update(benchmark::State& state) {
    Page p(1);
    (void)p.addRecord(Record(1));
    Record updated(1);
    (void)updated.setField("v", Json(2));

    for (auto _ : state) {
        (void)p.updateRecord(updated);
    }
}
BENCHMARK(record_update);

// Measures deleteRecord() soft-deleting a record. Re-adds the record
// before every timed call so there's always something to delete.
static void record_delete(benchmark::State& state) {
    Page p(1);

    for (auto _ : state) {
        (void)p.addRecord(Record(1));
        (void)p.deleteRecord(1);
    }
}
BENCHMARK(record_delete);

// Measures compact() reclaiming a page that's half soft-deleted.
static void compact(benchmark::State& state) {
    for (auto _ : state) {
        Page p(1);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)p.addRecord(Record(i));
        }
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; i += 2) {
            (void)p.deleteRecord(i);
        }
        (void)p.compact();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(compact);