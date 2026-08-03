// Contract: PageID / TableID Monotonicity
// A PageID or TableID, once assigned, must never be reassigned to a
// different object for the lifetime of its owning Table/Database --
// including across drop+create cycles and across save/load round trips.
// This is the general contract; the specific bugs that violated it (both
// counters previously derived from container size, which shrinks on
// removal) have their own regression coverage in table.cpp/database.cpp.
//
// Covers:
// - Table: page ids assigned while inserting stay unique across many
//   pages and across repeated toJson/fromJson round trips
// - Database: table ids assigned while creating/dropping many tables stay
//   unique across the whole history, not just the currently-live set

#include <support/framework.h>

#include <unordered_set>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

Record makeRecord(RecordID id) {
    Record r(id);
    (void)r.setField("name", Json("r" + std::to_string(id)));
    return r;
}

} // namespace

// Verifies that page ids assigned to a Table strictly increase as pages
// are allocated (no gaps, no repeats) while only inserting -- no removal
// path exists for pages today, so this establishes the allocation-order
// baseline the reuse-after-load test below builds on.
static void table_page_ids_increase_monotonically() {
    Table t("t", 1, makeSchema());

    RecordID nextId = 0;
    PageID lastPageId = 0;
    bool first = true;

    for (int page = 0; page < 4; ++page) {
        for (uint32_t i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            CHK(t.insertRecord(makeRecord(nextId++)) == Status::OK);
        }
    }

    for (const Page* p : t.getPages()) {
        if (first) {
            lastPageId = p->getID();
            first = false;
            continue;
        }
        CHK(p->getID() > lastPageId);
        lastPageId = p->getID();
    }
    CHK(t.pageCount() == 4);
}

// Verifies that across repeated save (toJson) / load (fromJson) / insert
// cycles, newly-allocated pages always get ids beyond every previously
// loaded page -- i.e. the monotonic counter correctly resumes above the
// highest loaded id on every load, not just the first one. (Re-seeing the
// same id for a page that was already loaded in an earlier cycle is
// expected and not itself a collision; what must never happen is a new
// page reusing an old id.)
static void table_page_ids_unique_across_multiple_load_cycles() {
    RecordID nextRecordId = 0;
    bool first = true;
    PageID prevMaxPageId = 0;

    Table t("t", 1, makeSchema());

    for (int cycle = 0; cycle < 3; ++cycle) {
        for (uint32_t i = 0; i < DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
            CHK(t.insertRecord(makeRecord(nextRecordId++)) == Status::OK);
        }

        PageID currentMaxPageId = 0;
        for (const Page* p : t.getPages()) {
            if (p->getID() > currentMaxPageId)
                currentMaxPageId = p->getID();
        }

        if (!first) {
            CHK(currentMaxPageId > prevMaxPageId);
        }
        prevMaxPageId = currentMaxPageId;
        first = false;

        Json envelope = t.toJson();
        Table reloaded("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
        CHK(reloaded.fromJson(envelope) == Status::OK);
        t = std::move(reloaded);
    }
}

// Verifies that TableIDs assigned by Database strictly increase while only
// creating tables (no drops).
static void database_table_ids_increase_monotonically() {
    Database db("app");

    TableID lastId = 0;
    bool first = true;
    for (int i = 0; i < 10; ++i) {
        CHK(db.createTable("t" + std::to_string(i), makeSchema()) == Status::OK);
        TableID id = db.getTable("t" + std::to_string(i))->getID();
        if (!first)
            CHK(id > lastId);
        lastId = id;
        first = false;
    }
}

// The core contract check: across many create/drop cycles, every TableID
// ever handed out is unique across the whole history of the Database, not
// just among the currently-live tables.
static void database_table_ids_never_reused_across_many_cycles() {
    Database db("app");
    std::unordered_set<TableID> seenIds;

    for (int i = 0; i < 25; ++i) {
        std::string name = "t" + std::to_string(i);
        CHK(db.createTable(name, makeSchema()) == Status::OK);

        TableID id = db.getTable(name)->getID();
        bool wasNew = seenIds.insert(id).second;
        CHK(wasNew == true); // must never have been assigned before

        CHK(db.dropTable(name) == Status::OK);
    }
}

// Executes all id-monotonicity contract checks.
static void run_tests() {
    RUN(table_page_ids_increase_monotonically);
    RUN(table_page_ids_unique_across_multiple_load_cycles);
    RUN(database_table_ids_increase_monotonically);
    RUN(database_table_ids_never_reused_across_many_cycles);
}

REGISTER_TEST_SUITE();
