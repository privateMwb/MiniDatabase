// Database Test Suite
// Verifies table management, id-collision safety, and whole-database
// persistence (save/load) for Database.
//
// Covers:
// - createTable / dropTable / getTable / hasTable
// - table-id collision safety after drop + create (regression coverage)
// - toJson / fromJson round trip
// - save / load round trip through real files
// - load() leaving the database untouched when given a corrupt file
//   (all-or-nothing load regression coverage)
// - compact()

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

Record makeRecord(RecordID id, const std::string& name) {
    Record r(id);
    (void)r.setField("name", Json(name));
    return r;
}

// Returns a fresh temp file path for this test run; not created yet.
std::string tempPath(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path();
    return (dir / ("minidb_test_" + label + ".json")).string();
}

} // namespace

// Verifies createTable succeeds and the table is reachable via getTable.
TEST(Database, CreateTableBasic) {
    Database db("app");

    ASSERT_EQ(db.createTable("users", makeSchema()), Status::OK);
    EXPECT_TRUE(db.hasTable("users"));

    Table* t = db.getTable("users");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->getName(), "users");
}

// Verifies creating a table with a name already in use fails.
TEST(Database, CreateDuplicateTableFails) {
    Database db("app");

    ASSERT_EQ(db.createTable("users", makeSchema()), Status::OK);
    EXPECT_EQ(db.createTable("users", makeSchema()), Status::TABLE_ALREADY_EXISTS);
}

// Verifies dropTable removes the table (hasTable/getTable reflect it).
TEST(Database, DropTableRemovesIt) {
    Database db("app");
    ASSERT_EQ(db.createTable("users", makeSchema()), Status::OK);

    ASSERT_EQ(db.dropTable("users"), Status::OK);
    EXPECT_FALSE(db.hasTable("users"));
    EXPECT_EQ(db.getTable("users"), nullptr);
}

// Verifies dropTable on a missing table returns TABLE_NOT_FOUND.
TEST(Database, DropMissingTableFails) {
    Database db("app");
    EXPECT_EQ(db.dropTable("ghost"), Status::TABLE_NOT_FOUND);
}

// Verifies getTable on a missing table returns nullptr rather than a
// dangling or default-constructed pointer.
TEST(Database, GetMissingTableReturnsNullptr) {
    Database db("app");
    EXPECT_EQ(db.getTable("ghost"), nullptr);
}

// Regression: creating a table after dropping another must not reuse the
// dropped table's id -- previously nextTableID() returned tables.size(),
// which collided with a surviving table's id after a drop, causing
// getTable() to resolve to the wrong table.
TEST(Database, TableIdNoCollisionAfterDropAndCreate) {
    Database db("app");
    ASSERT_EQ(db.createTable("a", makeSchema()), Status::OK);
    ASSERT_EQ(db.createTable("b", makeSchema()), Status::OK);
    ASSERT_EQ(db.createTable("c", makeSchema()), Status::OK);

    TableID idC = db.getTable("c")->getID();

    ASSERT_EQ(db.dropTable("b"), Status::OK);
    ASSERT_EQ(db.createTable("d", makeSchema()), Status::OK);

    // "c" must still resolve to itself, not be aliased by "d".
    EXPECT_EQ(db.getTable("c")->getID(), idC);
    EXPECT_EQ(db.getTable("c")->getName(), "c");
    EXPECT_EQ(db.getTable("d")->getName(), "d");
    EXPECT_NE(db.getTable("d")->getID(), idC);
}

// Verifies toJson()/fromJson() preserves every table and its records.
TEST(Database, JsonRoundTripPreservesData) {
    Database original("app");
    ASSERT_EQ(original.createTable("users", makeSchema()), Status::OK);
    ASSERT_EQ(original.getTable("users")->insertRecord(makeRecord(1, "Ada")), Status::OK);

    Json envelope = original.toJson();

    Database restored("");
    ASSERT_EQ(restored.fromJson(envelope), Status::OK);

    EXPECT_EQ(restored.getName(), "app");
    EXPECT_TRUE(restored.hasTable("users"));
    EXPECT_EQ(restored.recordCount(), 1);

    Record out;
    ASSERT_EQ(restored.getTable("users")->getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");
}

// Verifies a full save()/load() round trip through a real file.
TEST(Database, SaveLoadRoundTrip) {
    std::string path = tempPath("save_load");
    std::filesystem::remove(path);

    {
        Database original("app");
        ASSERT_EQ(original.createTable("users", makeSchema()), Status::OK);
        ASSERT_EQ(original.getTable("users")->insertRecord(makeRecord(1, "Ada")), Status::OK);
        ASSERT_EQ(original.save(path), Status::OK);
    }

    Database restored("");
    ASSERT_EQ(restored.load(path), Status::OK);
    EXPECT_TRUE(restored.hasTable("users"));

    Record out;
    ASSERT_EQ(restored.getTable("users")->getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");

    std::filesystem::remove(path);
}

// Regression: load() on a corrupt file must fail cleanly and must not
// leave the database in a partial state -- previously a mid-load failure
// left `tables` with only some tables loaded and the rest silently
// missing.
TEST(Database, LoadCorruptFileLeavesDatabaseUntouched) {
    std::string goodPath = tempPath("load_atomicity_good");
    std::string corruptPath = tempPath("load_atomicity_corrupt");
    std::filesystem::remove(goodPath);
    std::filesystem::remove(corruptPath);

    Database db("app");
    ASSERT_EQ(db.createTable("users", makeSchema()), Status::OK);
    ASSERT_EQ(db.getTable("users")->insertRecord(makeRecord(1, "Ada")), Status::OK);
    ASSERT_EQ(db.save(goodPath), Status::OK);

    {
        std::ofstream corrupt(corruptPath, std::ios::binary);
        corrupt << "{ this is not valid json";
    }

    Status s = db.load(corruptPath);
    ASSERT_NE(s, Status::OK);

    // The database must still reflect its pre-load-attempt state.
    EXPECT_TRUE(db.hasTable("users"));
    Record out;
    ASSERT_EQ(db.getTable("users")->getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");

    std::filesystem::remove(goodPath);
    std::filesystem::remove(corruptPath);
}

// Verifies load() on a nonexistent file returns a non-OK status rather
// than crashing.
TEST(Database, LoadMissingFileReturnsError) {
    Database db("app");
    EXPECT_NE(db.load(tempPath("does_not_exist_" + std::to_string(std::rand()))), Status::OK);
}

// Verifies compact() clears the dirty flag across all tables.
TEST(Database, CompactClearsDirty) {
    Database db("app");
    ASSERT_EQ(db.createTable("users", makeSchema()), Status::OK);
    ASSERT_EQ(db.getTable("users")->insertRecord(makeRecord(1, "Ada")), Status::OK);
    ASSERT_TRUE(db.isDirty());

    ASSERT_EQ(db.compact(), Status::OK);
    EXPECT_FALSE(db.isDirty());
}
