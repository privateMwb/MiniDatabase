// JSON Round Trip Benchmark Suite
// Measures the tree-based (toJson/fromJson) path against the
// string-based (serialize/deserialize) path for each Core class, showing
// the cost of the extra dump()/parse() step the string path pays.
//
// Covers:
// - Record: toJson/fromJson vs serialize/deserialize
// - Page: toJson/fromJson vs serialize/deserialize
// - Table: toJson/fromJson vs serialize/deserialize

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures Record's tree-based vs string-based round trip.
static void bench_record_roundtrip() {
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    (void)r.setField("age", Json(30));

    auto tree = [&] {
        Json j = r.toJson();
        Record out;
        (void)out.fromJson(j);
        doNotOptimize(out);
    };
    BENCH("Record toJson/fromJson", tree);
    std::cout << "\n";

    auto str = [&] {
        std::string raw = r.serialize();
        Record out;
        (void)out.deserialize(raw);
        doNotOptimize(out);
    };
    BENCH("Record serialize/deserialize", str);
}

// Measures Page's tree-based vs string-based round trip on a full page.
static void bench_page_roundtrip() {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        (void)p.addRecord(Record(i));
    }

    auto tree = [&] {
        Json j = p.toJson();
        Page out;
        (void)out.fromJson(j);
        doNotOptimize(out);
    };
    BENCH_CUSTOM("Page toJson/fromJson", tree);
    std::cout << "\n";

    auto str = [&] {
        std::string raw = p.serialize();
        Page out;
        (void)out.deserialize(raw);
        doNotOptimize(out);
    };
    BENCH_CUSTOM("Page serialize/deserialize", str);
}

// Measures Table's tree-based vs string-based round trip on a multi-page
// table.
static void bench_table_roundtrip() {
    Table t("orders", 1, Vector<ColumnDef>{});
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
        (void)t.insertRecord(Record(i));
    }

    auto tree = [&] {
        Json j = t.toJson();
        Table out("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
        (void)out.fromJson(j);
        doNotOptimize(out);
    };
    BENCH_CUSTOM("Table toJson/fromJson", tree);
    std::cout << "\n";

    auto str = [&] {
        std::string raw = t.serialize();
        Table out("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
        (void)out.deserialize(raw);
        doNotOptimize(out);
    };
    BENCH_CUSTOM("Table serialize/deserialize", str);
}

// Executes all JSON round-trip benchmark cases.
static void run_benchmarks() {
    bench_record_roundtrip();
    std::cout << "\n";

    bench_page_roundtrip();
    std::cout << "\n";

    bench_table_roundtrip();
}

REGISTER_BENCH_SUITE();