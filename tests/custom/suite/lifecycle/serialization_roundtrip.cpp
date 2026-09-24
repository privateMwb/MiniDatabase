// Contract: Serialization Path Equivalence
// Every serializable type exposes two equivalent paths:
//   - toJson() / fromJson(const Json&)      (in-memory tree, no string I/O)
//   - serialize() / deserialize(string)     (thin dump()/parse() wrapper
//                                             around the above, meant only
//                                             for the outermost I/O boundary)
// These two paths must reconstruct identical observable state. This
// contract exists specifically so that if either path's implementation
// ever drifts from the other (e.g. someone adds a field to toJson() but
// forgets serialize() is just a wrapper around it and adds a separate,
// inconsistent field elsewhere), a test catches it immediately rather than
// only showing up as a subtle data-loss bug in one path but not the other.
//
// Covers: Record, Page, Table, Database.

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

} // namespace

// Verifies Record's two reconstruction paths agree.
static void record_paths_agree() {
    Record original(3);
    (void)original.setField("name", Json("Ada"));

    Record viaJson;
    CHK(viaJson.fromJson(original.toJson()) == Status::OK);

    Record viaString;
    CHK(viaString.deserialize(original.serialize()) == Status::OK);

    CHK(viaJson.getID() == viaString.getID());
    CHK(viaJson.isDeleted() == viaString.isDeleted());
    CHK(viaJson.getField("name").asString() == viaString.getField("name").asString());
}

// Verifies Page's two reconstruction paths agree.
static void page_paths_agree() {
    Page original(9);
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    CHK(original.addRecord(r) == Status::OK);

    Page viaJson;
    CHK(viaJson.fromJson(original.toJson()) == Status::OK);

    Page viaString;
    CHK(viaString.deserialize(original.serialize()) == Status::OK);

    CHK(viaJson.getID() == viaString.getID());
    CHK(viaJson.recordCount() == viaString.recordCount());

    const Record* rj = viaJson.getRecord(1);
    const Record* rs = viaString.getRecord(1);
    CHK(rj != nullptr);
    CHK(rs != nullptr);
    CHK(rj->getField("name").asString() == rs->getField("name").asString());
}

// Verifies Table's two reconstruction paths agree.
static void table_paths_agree() {
    Table original("users", 1, makeSchema());
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    CHK(original.insertRecord(r) == Status::OK);

    Table viaJson("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(viaJson.fromJson(original.toJson()) == Status::OK);

    Table viaString("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(viaString.deserialize(original.serialize()) == Status::OK);

    CHK(viaJson.getName() == viaString.getName());
    CHK(viaJson.getID() == viaString.getID());
    CHK(viaJson.recordCount() == viaString.recordCount());

    Record outJson, outString;
    CHK(viaJson.getRecord(1, outJson) == Status::OK);
    CHK(viaString.getRecord(1, outString) == Status::OK);
    CHK(outJson.getField("name").asString() == outString.getField("name").asString());
}

// Verifies Database's two reconstruction paths agree.
static void database_paths_agree() {
    Database original("app");
    CHK(original.createTable("users", makeSchema()) == Status::OK);
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    CHK(original.getTable("users")->insertRecord(r) == Status::OK);

    Database viaJson("");
    CHK(viaJson.fromJson(original.toJson()) == Status::OK);

    CHK(viaJson.getName() == "app");
    CHK(viaJson.hasTable("users") == true);
    CHK(viaJson.recordCount() == 1);

    // Database has no separate string serialize()/deserialize() of its
    // own -- save()/load() go through toJson()/fromJson() plus FileIO, so
    // there is no second path to cross-check here beyond what
    // database.cpp's save/load round-trip test already covers.
}

// Executes all serialization-path-equivalence contract checks.
static void run_tests() {
    RUN(record_paths_agree);
    RUN(page_paths_agree);
    RUN(table_paths_agree);
    RUN(database_paths_agree);
}

REGISTER_TEST_SUITE();
