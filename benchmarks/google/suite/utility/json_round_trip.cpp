// JSON Round Trip Benchmark Suite
// Measures the tree-based (toJson/fromJson) path against the
// string-based (serialize/deserialize) path for each Core class, showing
// the cost of the extra dump()/parse() step the string path pays.
//
// Covers:
// - Record: toJson/fromJson vs serialize/deserialize
// - Page: toJson/fromJson vs serialize/deserialize
// - Table: toJson/fromJson vs serialize/deserialize

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Measures Record's tree-based round trip.
static void record_roundtrip_tree(benchmark::State& state) {
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    (void)r.setField("age", Json(30));

    for (auto _ : state) {
        Json j = r.toJson();
        Record out;
        (void)out.fromJson(j);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(record_roundtrip_tree);

// Measures Record's string-based round trip, for direct contrast against
// the tree-based path above.
static void record_roundtrip_str(benchmark::State& state) {
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    (void)r.setField("age", Json(30));

    for (auto _ : state) {
        std::string raw = r.serialize();
        Record out;
        (void)out.deserialize(raw);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(record_roundtrip_str);

// Measures Page's tree-based round trip on a full page.
static void page_roundtrip_tree(benchmark::State& state) {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        (void)p.addRecord(Record(i));
    }

    for (auto _ : state) {
        Json j = p.toJson();
        Page out;
        (void)out.fromJson(j);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(page_roundtrip_tree);

// Measures Page's string-based round trip on a full page, for direct
// contrast against the tree-based path above.
static void page_roundtrip_str(benchmark::State& state) {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        (void)p.addRecord(Record(i));
    }

    for (auto _ : state) {
        std::string raw = p.serialize();
        Page out;
        (void)out.deserialize(raw);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(page_roundtrip_str);

// Measures Table's tree-based round trip on a multi-page table.
static void table_roundtrip_tree(benchmark::State& state) {
    Table t("orders", 1, Vector<ColumnDef>{});
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
        (void)t.insertRecord(Record(i));
    }

    for (auto _ : state) {
        Json j = t.toJson();
        Table out("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
        (void)out.fromJson(j);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(table_roundtrip_tree);

// Measures Table's string-based round trip on a multi-page table, for
// direct contrast against the tree-based path above.
static void table_roundtrip_str(benchmark::State& state) {
    Table t("orders", 1, Vector<ColumnDef>{});
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
        (void)t.insertRecord(Record(i));
    }

    for (auto _ : state) {
        std::string raw = t.serialize();
        Table out("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
        (void)out.deserialize(raw);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(table_roundtrip_str);