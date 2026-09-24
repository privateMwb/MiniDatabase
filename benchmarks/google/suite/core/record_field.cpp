// Record Field Benchmark Suite
// Measures Record's field-access API: setField, getField (copies),
// getFieldRef (the non-copying hot-path accessor), hasField, and
// removeField.
//
// Covers:
// - setField
// - getField (existing key)
// - getFieldRef (existing key)
// - hasField
// - removeField

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures setField() overwriting an existing key.
static void field_set(benchmark::State& state) {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    for (auto _ : state) {
        (void)r.setField("name", Json("Grace"));
    }
}
BENCHMARK(field_set);

// Measures getField() copying the value out.
static void field_get(benchmark::State& state) {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    for (auto _ : state) {
        Json v = r.getField("name");
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(field_get);

// Measures getFieldRef() -- the non-copying accessor, contrasted directly
// against getField() above.
static void field_getref(benchmark::State& state) {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    for (auto _ : state) {
        const Json& v = r.getFieldRef("name");
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(field_getref);

// Measures hasField() on an existing key.
static void field_has(benchmark::State& state) {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    for (auto _ : state) {
        bool b = r.hasField("name");
        benchmark::DoNotOptimize(b);
    }
}
BENCHMARK(field_has);

// Measures removeField() on an existing key. Re-sets the field before
// every timed call so removeField always has something to remove.
static void field_remove(benchmark::State& state) {
    Record r(1);

    for (auto _ : state) {
        (void)r.setField("name", Json("Ada"));
        (void)r.removeField("name");
    }
}
BENCHMARK(field_remove);