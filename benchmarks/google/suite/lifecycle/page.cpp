// Page Construction Benchmark Suite
// Measures Page's constructors and its move-only transfer cost, both
// empty and holding a full page of records -- Page is not copyable, so
// only move is meaningful here.
//
// Covers:
// - default constructor
// - explicit-id constructor
// - move construction, empty page
// - move construction, full page

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures the default constructor.
static void page_ctor_default(benchmark::State& state) {
    for (auto _ : state) {
        Page p;
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(page_ctor_default);

// Measures the explicit-id constructor.
static void page_ctor_id(benchmark::State& state) {
    for (auto _ : state) {
        Page p(1);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(page_ctor_id);

// Measures move construction of an empty page.
static void page_move_empty(benchmark::State& state) {
    for (auto _ : state) {
        Page source(1);
        Page p(std::move(source));
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(page_move_empty);

// Measures move construction of a full page -- shows whether move cost
// scales with record count or stays flat (pointer/handle transfer).
static void page_move_full(benchmark::State& state) {
    for (auto _ : state) {
        Page source(1);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)source.addRecord(Record(i));
        }
        Page p(std::move(source));
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(page_move_full);