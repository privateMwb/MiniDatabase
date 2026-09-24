// Record Construction Benchmark Suite
// Measures Record's three constructors plus copy vs move, since Record
// (unlike Page/Table/Database) is copyable -- its Json data member can be
// deep-copied, so copy cost is worth contrasting with move cost directly.
//
// Covers:
// - default constructor
// - id-only constructor
// - id + data constructor
// - copy construction
// - move construction

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures the default constructor.
static void record_ctor_default(benchmark::State& state) {
    for (auto _ : state) {
        Record r;
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(record_ctor_default);

// Measures the id-only constructor.
static void record_ctor_id(benchmark::State& state) {
    for (auto _ : state) {
        Record r(1);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(record_ctor_id);

// Measures the id + data constructor with a handful of fields, the
// realistic shape a freshly-inserted record takes.
static void record_ctor_data(benchmark::State& state) {
    Json data(Json::ObjectType{});
    data["name"] = Json("Ada");
    data["age"] = Json(30);

    for (auto _ : state) {
        Record r(1, data);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(record_ctor_data);

// Measures copy construction of a record holding a few fields.
static void record_copy(benchmark::State& state) {
    Record source(1);
    (void)source.setField("name", Json("Ada"));
    (void)source.setField("age", Json(30));

    for (auto _ : state) {
        Record r(source);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(record_copy);

// Measures move construction of the same shape -- the contrast point
// against copy.
static void record_move(benchmark::State& state) {
    for (auto _ : state) {
        Record source(1);
        (void)source.setField("name", Json("Ada"));
        (void)source.setField("age", Json(30));
        Record r(std::move(source));
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(record_move);