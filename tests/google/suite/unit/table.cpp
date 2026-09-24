// Table Test Suite
// Verifies CRUD, schema validation propagation, page allocation, and
// serialization for a single Table in isolation (no Database, no file I/O).
//
// Covers:
// - insertRecord / getRecord / updateRecord / deleteRecord
// - schema introspection (getSchema / hasColumn)
// - page allocation across the MAX_RECORDS_PAGE boundary
// - compact() / rebuildIndex()
// - toJson / fromJson round trip, including that the page-id counter
//   resumes above the highest loaded page id after a load (regression
//   coverage for the id-collision-after-reuse bug)
// - serialize / deserialize round trip
// - deserialize on malformed input

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"score", ColumnType::INT, true},
    };
}

Record makeRecord(RecordID id, const std::string& name) {
    Record r(id);
    (void)r.setField("name", Json(name));
    return r;
}

} // namespace

// Verifies basic construction and introspection getters.
TEST(Table, ConstructBasic) {
    Table t("users", 1, makeSchema());

    EXPECT_EQ(t.getName(), "users");
    EXPECT_EQ(t.getID(), 1);
    EXPECT_TRUE(t.isEmpty());
    EXPECT_EQ(t.recordCount(), 0);
    EXPECT_EQ(t.pageCount(), 0);
}

// Verifies hasColumn against the schema passed at construction.
TEST(Table, HasColumnTrueFalse) {
    Table t("users", 1, makeSchema());

    EXPECT_TRUE(t.hasColumn("name"));
    EXPECT_TRUE(t.hasColumn("score"));
    EXPECT_FALSE(t.hasColumn("missing"));
}

// Verifies insertRecord followed by getRecord returns the same data.
TEST(Table, InsertAndGet) {
    Table t("users", 1, makeSchema());

    ASSERT_EQ(t.insertRecord(makeRecord(1, "Ada")), Status::OK);

    Record out;
    ASSERT_EQ(t.getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");
}

// Verifies that inserting a record with an id already present fails with
// DUPLICATE_KEY rather than overwriting.
TEST(Table, InsertDuplicateFails) {
    Table t("users", 1, makeSchema());

    ASSERT_EQ(t.insertRecord(makeRecord(1, "Ada")), Status::OK);
    EXPECT_EQ(t.insertRecord(makeRecord(1, "Grace")), Status::DUPLICATE_KEY);
}

// Verifies that schema validation failures from Record::validate propagate
// through insertRecord rather than being silently accepted.
TEST(Table, InsertInvalidSchemaFails) {
    Table t("users", 1, makeSchema());

    Record bad(1);
    (void)bad.setField("name", Json(123)); // STRING column holding a number

    ASSERT_NE(t.insertRecord(bad), Status::OK);
    EXPECT_EQ(t.recordCount(), 0);
}

// Verifies getRecord on a missing id returns NOT_FOUND.
TEST(Table, GetMissingReturnsNotFound) {
    Table t("users", 1, makeSchema());

    Record out;
    EXPECT_EQ(t.getRecord(999, out), Status::NOT_FOUND);
}

// Verifies updateRecord replaces existing data.
TEST(Table, UpdateExistingReplacesData) {
    Table t("users", 1, makeSchema());
    ASSERT_EQ(t.insertRecord(makeRecord(1, "Ada")), Status::OK);

    ASSERT_EQ(t.updateRecord(makeRecord(1, "Grace")), Status::OK);

    Record out;
    ASSERT_EQ(t.getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Grace");
}

// Verifies updateRecord on a missing id returns NOT_FOUND.
TEST(Table, UpdateMissingReturnsNotFound) {
    Table t("users", 1, makeSchema());
    EXPECT_EQ(t.updateRecord(makeRecord(1, "Ada")), Status::NOT_FOUND);
}

// Verifies deleteRecord removes the record from the index (subsequent
// getRecord returns NOT_FOUND) and reports NOT_FOUND on a missing id.
TEST(Table, DeleteExistingAndMissing) {
    Table t("users", 1, makeSchema());
    ASSERT_EQ(t.insertRecord(makeRecord(1, "Ada")), Status::OK);

    ASSERT_EQ(t.deleteRecord(1), Status::OK);

    Record out;
    EXPECT_EQ(t.getRecord(1, out), Status::NOT_FOUND);
    EXPECT_EQ(t.deleteRecord(1), Status::NOT_FOUND);
}

// Verifies that inserting more than MAX_RECORDS_PAGE records allocates a
// second page, and every record remains reachable via getRecord (exercises
// the O(1) PageID -> Page* index across multiple pages).
TEST(Table, OverflowAllocatesSecondPage) {
    Table t("users", 1, makeSchema());

    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
        ASSERT_EQ(t.insertRecord(makeRecord(i, "user_" + std::to_string(i))), Status::OK);
    }

    EXPECT_EQ(t.pageCount(), 2);
    EXPECT_EQ(t.recordCount(), DBConstants::MAX_RECORDS_PAGE + 1);

    Record out;
    EXPECT_EQ(t.getRecord(0, out), Status::OK);
    EXPECT_EQ(t.getRecord(DBConstants::MAX_RECORDS_PAGE, out), Status::OK);
}

// Verifies compact() clears the dirty flag.
TEST(Table, CompactClearsDirty) {
    Table t("users", 1, makeSchema());
    ASSERT_EQ(t.insertRecord(makeRecord(1, "Ada")), Status::OK);
    ASSERT_TRUE(t.isDirty());

    ASSERT_EQ(t.compact(), Status::OK);
    EXPECT_FALSE(t.isDirty());
}

// Verifies rebuildIndex() reconstructs lookups correctly (records remain
// retrievable after an explicit rebuild).
TEST(Table, RebuildIndexPreservesLookups) {
    Table t("users", 1, makeSchema());
    ASSERT_EQ(t.insertRecord(makeRecord(1, "Ada")), Status::OK);
    ASSERT_EQ(t.insertRecord(makeRecord(2, "Grace")), Status::OK);

    ASSERT_EQ(t.rebuildIndex(), Status::OK);

    Record out;
    ASSERT_EQ(t.getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");
    ASSERT_EQ(t.getRecord(2, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Grace");
}

// Verifies toJson()/fromJson() preserves schema and all records, and that
// getRecord works immediately after fromJson() without a separate
// rebuildIndex() call.
TEST(Table, JsonRoundTripPreservesData) {
    Table original("users", 1, makeSchema());
    ASSERT_EQ(original.insertRecord(makeRecord(1, "Ada")), Status::OK);
    ASSERT_EQ(original.insertRecord(makeRecord(2, "Grace")), Status::OK);

    Json envelope = original.toJson();

    Table restored("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    ASSERT_EQ(restored.fromJson(envelope), Status::OK);

    EXPECT_EQ(restored.getName(), "users");
    EXPECT_EQ(restored.recordCount(), 2);

    Record out;
    ASSERT_EQ(restored.getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");
    ASSERT_EQ(restored.getRecord(2, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Grace");
}

// Regression: after loading a table via fromJson(), the page-id counter
// must resume above the highest loaded page id, so a newly-allocated page
// can never collide with one restored from disk.
TEST(Table, PageIdResumesAfterLoad) {
    Table original("users", 1, makeSchema());
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
        ASSERT_EQ(original.insertRecord(makeRecord(i, "user_" + std::to_string(i))), Status::OK);
    }
    ASSERT_EQ(original.pageCount(), 2);

    PageID maxLoadedPageId = 0;
    for (const Page* p : original.getPages()) {
        if (p->getID() > maxLoadedPageId)
            maxLoadedPageId = p->getID();
    }

    Json envelope = original.toJson();
    Table restored("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    ASSERT_EQ(restored.fromJson(envelope), Status::OK);

    // Force allocation of a brand new page beyond what was loaded.
    for (RecordID i = 1000; i < 1000 + DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
        ASSERT_EQ(restored.insertRecord(makeRecord(i, "extra")), Status::OK);
    }

    bool sawNewPageIdBeyondLoaded = false;
    for (const Page* p : restored.getPages()) {
        if (p->getID() > maxLoadedPageId)
            sawNewPageIdBeyondLoaded = true;
    }
    EXPECT_TRUE(sawNewPageIdBeyondLoaded);
}

// Verifies serialize()/deserialize() round trip matches toJson()/fromJson().
TEST(Table, SerializeRoundTrip) {
    Table original("users", 1, makeSchema());
    ASSERT_EQ(original.insertRecord(makeRecord(1, "Ada")), Status::OK);

    std::string raw = original.serialize();

    Table restored("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    ASSERT_EQ(restored.deserialize(raw), Status::OK);

    EXPECT_EQ(restored.getName(), "users");
    Record out;
    ASSERT_EQ(restored.getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");
}

// Verifies deserialize() on malformed JSON returns PARSE_ERROR.
TEST(Table, DeserializeMalformedReturnsParseError) {
    Table t("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    EXPECT_EQ(t.deserialize("{not valid json"), Status::PARSE_ERROR);
}
