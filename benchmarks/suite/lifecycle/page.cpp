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

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures the default constructor.
static void bench_default_ctor() {
    auto ctor = [] {
        Page p;
        doNotOptimize(p);
    };
    BENCH("Page default ctor", ctor);
}

// Measures the explicit-id constructor.
static void bench_id_ctor() {
    auto ctor = [] {
        Page p(1);
        doNotOptimize(p);
    };
    BENCH("Page id ctor", ctor);
}

// Measures move construction of an empty page.
static void bench_move_empty() {
    auto move = [] {
        Page source(1);
        Page p(std::move(source));
        doNotOptimize(p);
    };
    BENCH("Page move ctor (empty)", move);
}

// Measures move construction of a full page -- shows whether move cost
// scales with record count or stays flat (pointer/handle transfer).
static void bench_move_full() {
    auto move = [] {
        Page source(1);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            (void)source.addRecord(Record(i));
        }
        Page p(std::move(source));
        doNotOptimize(p);
    };
    BENCH("Page move ctor (full)", move);
}

// Executes all Page construction benchmark cases.
static void run_benchmarks() {
    bench_default_ctor();
    std::cout << "\n";

    bench_id_ctor();
    std::cout << "\n";

    bench_move_empty();
    std::cout << "\n";

    bench_move_full();
}

REGISTER_BENCH_SUITE();