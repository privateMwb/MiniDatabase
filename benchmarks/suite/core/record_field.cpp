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
    BENCH("Record setField", set);
}

// Measures getField() copying the value out.
static void bench_get_field() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    auto get = [&] {
        Json v = r.getField("name");
        doNotOptimize(v);
    };
    BENCH("Record getField", get);
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
    BENCH("Record getFieldRef", get);
}

// Measures hasField() on an existing key.
static void bench_has_field() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));

    auto has = [&] {
        bool b = r.hasField("name");
        doNotOptimize(b);
    };
    BENCH("Record hasField", has);
}

// Measures removeField() on an existing key. Re-sets the field before
// every timed call so removeField always has something to remove.
static void bench_remove_field() {
    Record r(1);

    auto remove = [&] {
        (void)r.setField("name", Json("Ada"));
        (void)r.removeField("name");
    };
    BENCH("Record removeField", remove);
}

// Executes all Record field benchmark cases.
static void run_benchmarks() {
    bench_set_field();
    std::cout << "\n";

    bench_get_field();
    std::cout << "\n";

    bench_get_field_ref();
    std::cout << "\n";

    bench_has_field();
    std::cout << "\n";

    bench_remove_field();
}

REGISTER_BENCH_SUITE();