// Table Mutation Benchmark Suite
// Measures Table's record-lifecycle mutators on a multi-page table:
// insertRecord, updateRecord, deleteRecord, and rebuildIndex.
//
// Covers:
// - insertRecord (into an existing multi-page table)
// - updateRecord (existing record)
// - deleteRecord
// - rebuildIndex

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

constexpr RecordID kSeedCount = DBConstants::MAX_RECORDS_PAGE * 10;

// Builds a table with kSeedCount records already inserted.
static Table makeSeededTable() {
    Table t("orders", 1, Vector<ColumnDef>{});
    for (RecordID i = 0; i < kSeedCount; ++i) {
        (void)t.insertRecord(Record(i));
    }
    return t;
}

// Measures insertRecord() adding a new, never-before-seen id into an
// already multi-page table.
static void table_record_insert(benchmark::State& state) {
    Table t = makeSeededTable();
    RecordID nextId = kSeedCount;

    for (auto _ : state) {
        (void)t.insertRecord(Record(nextId++));
    }
}
BENCHMARK(table_record_insert);

// Measures updateRecord() replacing an existing record's data, looked up
// through the index.
static void table_record_update(benchmark::State& state) {
    Table t = makeSeededTable();
    Record updated(kSeedCount / 2);
    (void)updated.setField("v", Json(1));

    for (auto _ : state) {
        (void)t.updateRecord(updated);
    }
}
BENCHMARK(table_record_update);

// Measures deleteRecord() on an existing record, looked up through the
// index.
static void table_record_delete(benchmark::State& state) {
    Table t = makeSeededTable();
    RecordID id = 0;

    for (auto _ : state) {
        (void)t.deleteRecord(id++);
    }
}
BENCHMARK(table_record_delete);

// Measures rebuildIndex() over a multi-page table.
static void rebuild_index(benchmark::State& state) {
    Table t = makeSeededTable();

    for (auto _ : state) {
        (void)t.rebuildIndex();
    }
}
BENCHMARK(rebuild_index);