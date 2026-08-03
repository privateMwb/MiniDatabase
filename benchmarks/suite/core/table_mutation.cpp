// Table Mutation Benchmark Suite
// Measures Table's record-lifecycle mutators on a multi-page table:
// insertRecord, updateRecord, deleteRecord, and rebuildIndex.
//
// Covers:
// - insertRecord (into an existing multi-page table)
// - updateRecord (existing record)
// - deleteRecord
// - rebuildIndex

#include <support/framework.h>

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
static void bench_insert_record() {
    Table t = makeSeededTable();
    RecordID nextId = kSeedCount;

    auto insert = [&] { (void)t.insertRecord(Record(nextId++)); };
    BENCH_CUSTOM("Table insertRecord", insert);
}

// Measures updateRecord() replacing an existing record's data, looked up
// through the index.
static void bench_update_record() {
    Table t = makeSeededTable();
    Record updated(kSeedCount / 2);
    (void)updated.setField("v", Json(1));

    auto update = [&] { (void)t.updateRecord(updated); };
    BENCH("Table updateRecord", update);
}

// Measures deleteRecord() on an existing record, looked up through the
// index.
static void bench_delete_record() {
    Table t = makeSeededTable();
    RecordID id = 0;

    auto del = [&] { (void)t.deleteRecord(id++); };
    BENCH("Table deleteRecord", del);
}

// Measures rebuildIndex() over a multi-page table.
static void bench_rebuild_index() {
    Table t = makeSeededTable();

    auto rebuild = [&] { (void)t.rebuildIndex(); };
    BENCH_CUSTOM("Table rebuildIndex", rebuild);
}

// Executes all Table mutation benchmark cases.
static void run_benchmarks() {
    bench_insert_record();
    std::cout << "\n";

    bench_update_record();
    std::cout << "\n";

    bench_delete_record();
    std::cout << "\n";

    bench_rebuild_index();
}

REGISTER_BENCH_SUITE();