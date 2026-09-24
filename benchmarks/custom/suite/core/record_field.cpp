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

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures setField() overwriting an existing key.
static void bench_set_field() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    auto set = [&] { (void)r.setField("name", Json("Grace")); };
    BENCH_SOLO("setField", set);
}

// Measures getField() copying the value out.
static void bench_get_field() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    auto get = [&] {
        Json v = r.getField("name");
        doNotOptimize(v);
    };
    BENCH_SOLO("getField", get);
}

// Measures getFieldRef() -- the non-copying accessor, contrasted directly
// against getField() above.
static void bench_get_field_ref() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    auto get = [&] {
        const Json& v = r.getFieldRef("name");
        doNotOptimize(v);
    };
    BENCH_SOLO("getFieldRef", get);
}

// Measures hasField() on an existing key.
static void bench_has_field() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    auto has = [&] {
        bool b = r.hasField("name");
        doNotOptimize(b);
    };
    BENCH_SOLO("hasField", has);
}

// Measures removeField() on an existing key. Re-sets the field before
// every timed call so removeField always has something to remove.
static void bench_remove_field() {
    Record r(1);

    auto remove = [&] {
        (void)r.setField("name", Json("Ada"));
        (void)r.removeField("name");
    };
    BENCH_SOLO("removeField", remove);
}

// Executes all Record field benchmark cases.
static void run_benchmarks() {
    bench_set_field();
    bench_get_field();
    bench_get_field_ref();
    bench_has_field();
    bench_remove_field();
}

REGISTER_BENCH_SUITE();