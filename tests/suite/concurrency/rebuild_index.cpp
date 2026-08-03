// Concurrency Rebuild Index Test Suite
// Verifies rebuildAllIndexesParallel() actually rebuilds EVERY table's
// index in a Database when run through the shared thread pool -- i.e.
// that runParallel() doesn't drop or skip any table's task, at both small
// scale and a scale exceeding the pool's thread count.
//
// Covers:
// - every record in every table (including one spanning multiple pages)
//   is still resolvable via getRecord() after a parallel rebuild
// - a database with zero tables is a clean no-op, not a crash
// - correctness at a table count (12) exceeding the default pool size
// - tables that exist but hold zero records don't misbehave

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Verifies that after rebuildAllIndexesParallel(), every record inserted
// into every table (including ones spanning multiple pages) is still
// resolvable via getRecord() -- confirming index and pageIndex were
// correctly rebuilt for each table independently.
static void rebuild_preserves_lookups() {
    Database db("shop");

    CHK(db.createTable("small", Vector<ColumnDef>{}) == Status::OK);
    for (RecordID i = 0; i < 5; ++i) {
        CHK(db.getTable("small")->insertRecord(Record(i)) == Status::OK);
    }

    CHK(db.createTable("spanning", Vector<ColumnDef>{}) == Status::OK);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 10; ++i) {
        CHK(db.getTable("spanning")->insertRecord(Record(i)) == Status::OK);
    }

    CHK(db.createTable("empty", Vector<ColumnDef>{}) == Status::OK);

    Concurrency conc;
    CHK(conc.rebuildAllIndexesParallel(db) == Status::OK);

    Record out;
    for (RecordID i = 0; i < 5; ++i) {
        CHK(db.getTable("small")->getRecord(i, out) == Status::OK);
    }
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 10; ++i) {
        CHK(db.getTable("spanning")->getRecord(i, out) == Status::OK);
    }
    CHK(db.getTable("empty")->recordCount() == 0);
}

// Verifies rebuildAllIndexesParallel() on a database with zero tables is a
// clean no-op (Status::OK), not a crash on an empty futures vector.
static void rebuild_empty_db() {
    Database db("empty_db");
    Concurrency conc;
    CHK(conc.rebuildAllIndexesParallel(db) == Status::OK);
}

// Verifies correctness at a scale (12 tables, each with a full page) where
// the number of tasks exceeds the default thread pool size -- makes sure
// every future is collected, not just the first N that happen to fit the
// pool's thread count.
static void rebuild_many_tables() {
    Database db("shop");
    constexpr int kTableCount = 12;

    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        CHK(db.createTable(name, Vector<ColumnDef>{}) == Status::OK);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            CHK(db.getTable(name)->insertRecord(Record(i)) == Status::OK);
        }
    }

    Concurrency conc;
    CHK(conc.rebuildAllIndexesParallel(db) == Status::OK);

    Record out;
    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            CHK(db.getTable(name)->getRecord(i, out) == Status::OK);
        }
    }
}

// Verifies rebuildAllIndexesParallel() doesn't crash or misbehave on
// tables that exist but hold zero records (zero pages) -- distinct from
// the zero-TABLES case already covered by rebuild_empty_db().
static void rebuild_empty_tables() {
    Database db("shop");
    CHK(db.createTable("a", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("b", Vector<ColumnDef>{}) == Status::OK);

    Concurrency conc;
    CHK(conc.rebuildAllIndexesParallel(db) == Status::OK);

    CHK(db.getTable("a")->recordCount() == 0);
    CHK(db.getTable("b")->recordCount() == 0);
}

static void run_tests() {
    RUN(rebuild_preserves_lookups);
    RUN(rebuild_empty_db);
    RUN(rebuild_many_tables);
    RUN(rebuild_empty_tables);
}

REGISTER_TEST_SUITE();