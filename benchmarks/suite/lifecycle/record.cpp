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

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures the default constructor.
static void bench_default_ctor() {
    auto ctor = [] {
        Record r;
        doNotOptimize(r);
    };
    BENCH("Record default ctor", ctor);
}

// Measures the id-only constructor.
static void bench_id_ctor() {
    auto ctor = [] {
        Record r(1);
        doNotOptimize(r);
    };
    BENCH("Record id ctor", ctor);
}

// Measures the id + data constructor with a handful of fields, the
// realistic shape a freshly-inserted record takes.
static void bench_data_ctor() {
    Json data(Json::ObjectType{});
    data["name"] = Json("Ada");
    data["age"] = Json(30);

    auto ctor = [&] {
        Record r(1, data);
        doNotOptimize(r);
    };
    BENCH("Record id+data ctor", ctor);
}

// Measures copy construction of a record holding a few fields.
static void bench_copy() {
    Record source(1);
    (void)source.setField("name", Json("Ada"));
    (void)source.setField("age", Json(30));

    auto copy = [&] {
        Record r(source);
        doNotOptimize(r);
    };
    BENCH("Record copy ctor", copy);
}

// Measures move construction of the same shape -- the contrast point
// against bench_copy.
static void bench_move() {
    auto move = [] {
        Record source(1);
        (void)source.setField("name", Json("Ada"));
        (void)source.setField("age", Json(30));
        Record r(std::move(source));
        doNotOptimize(r);
    };
    BENCH("Record move ctor", move);
}

// Executes all Record construction benchmark cases.
static void run_benchmarks() {
    bench_default_ctor();
    std::cout << "\n";

    bench_id_ctor();
    std::cout << "\n";

    bench_data_ctor();
    std::cout << "\n";

    bench_copy();
    std::cout << "\n";

    bench_move();
}

REGISTER_BENCH_SUITE();