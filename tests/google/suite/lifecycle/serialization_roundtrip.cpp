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

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

} // namespace

// Verifies Record's two reconstruction paths agree.
TEST(SerializationRoundtrip, RecordPathsAgree) {
    Record original(3);
    (void)original.setField("name", Json("Ada"));

    Record viaJson;
    ASSERT_EQ(viaJson.fromJson(original.toJson()), Status::OK);

    Record viaString;
    ASSERT_EQ(viaString.deserialize(original.serialize()), Status::OK);

    EXPECT_EQ(viaJson.getID(), viaString.getID());
    EXPECT_EQ(viaJson.isDeleted(), viaString.isDeleted());
    EXPECT_EQ(viaJson.getField("name").asString(), viaString.getField("name").asString());
}

// Verifies Page's two reconstruction paths agree.
TEST(SerializationRoundtrip, PagePathsAgree) {
    Page original(9);
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    ASSERT_EQ(original.addRecord(r), Status::OK);

    Page viaJson;
    ASSERT_EQ(viaJson.fromJson(original.toJson()), Status::OK);

    Page viaString;
    ASSERT_EQ(viaString.deserialize(original.serialize()), Status::OK);

    EXPECT_EQ(viaJson.getID(), viaString.getID());
    EXPECT_EQ(viaJson.recordCount(), viaString.recordCount());

    const Record* rj = viaJson.getRecord(1);
    const Record* rs = viaString.getRecord(1);
    ASSERT_NE(rj, nullptr);
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(rj->getField("name").asString(), rs->getField("name").asString());
}

// Verifies Table's two reconstruction paths agree.
TEST(SerializationRoundtrip, TablePathsAgree) {
    Table original("users", 1, makeSchema());
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    ASSERT_EQ(original.insertRecord(r), Status::OK);

    Table viaJson("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    ASSERT_EQ(viaJson.fromJson(original.toJson()), Status::OK);

    Table viaString("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    ASSERT_EQ(viaString.deserialize(original.serialize()), Status::OK);

    EXPECT_EQ(viaJson.getName(), viaString.getName());
    EXPECT_EQ(viaJson.getID(), viaString.getID());
    EXPECT_EQ(viaJson.recordCount(), viaString.recordCount());

    Record outJson, outString;
    ASSERT_EQ(viaJson.getRecord(1, outJson), Status::OK);
    ASSERT_EQ(viaString.getRecord(1, outString), Status::OK);
    EXPECT_EQ(outJson.getField("name").asString(), outString.getField("name").asString());
}

// Verifies Database's two reconstruction paths agree.
TEST(SerializationRoundtrip, DatabasePathsAgree) {
    Database original("app");
    ASSERT_EQ(original.createTable("users", makeSchema()), Status::OK);
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    ASSERT_EQ(original.getTable("users")->insertRecord(r), Status::OK);

    Database viaJson("");
    ASSERT_EQ(viaJson.fromJson(original.toJson()), Status::OK);

    EXPECT_EQ(viaJson.getName(), "app");
    EXPECT_TRUE(viaJson.hasTable("users"));
    EXPECT_EQ(viaJson.recordCount(), 1);

    // Database has no separate string serialize()/deserialize() of its
    // own -- save()/load() go through toJson()/fromJson() plus FileIO, so
    // there is no second path to cross-check here beyond what
    // database.cpp's save/load round-trip test already covers.
}
