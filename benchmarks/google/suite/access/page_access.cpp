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

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

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
static void get_record_hit(benchmark::State& state) {
    Page p = makeFullPage();
    RecordID lastId = DBConstants::MAX_RECORDS_PAGE - 1;

    for (auto _ : state) {
        Record* r = p.getRecord(lastId);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(get_record_hit);

// Measures getRecord() scanning an entire full page for an id that doesn't
// exist -- the worst case for a miss.
static void get_record_miss(benchmark::State& state) {
    Page p = makeFullPage();

    for (auto _ : state) {
        Record* r = p.getRecord(9999);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(get_record_miss);

// Measures getRecordAt() at the same position -- direct index access, no
// scan required.
static void get_at_hit(benchmark::State& state) {
    Page p = makeFullPage();
    std::size_t lastIndex = DBConstants::MAX_RECORDS_PAGE - 1;

    for (auto _ : state) {
        Record* r = p.getRecordAt(lastIndex);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(get_at_hit);

// Measures getRecordAt() past the end of the page -- a bounds check, no
// scan required even on miss.
static void get_at_miss(benchmark::State& state) {
    Page p = makeFullPage();

    for (auto _ : state) {
        Record* r = p.getRecordAt(9999);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(get_at_miss);