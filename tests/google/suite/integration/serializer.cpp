// Integration: Serializer
// Exercises Serializer's table- and database-level export/import against
// real files, as opposed to Database's own save()/load() (which uses
// toJson()/fromJson() directly, not the Serializer's id-embedded-in-record
// JSON array format). Covers a genuinely different on-disk shape and
// import path than database_save_load.cpp.
//
// Covers:
// - exportTableToJson / importTableFromJson (string path) round trip
// - exportTableToFile / importTableFromFile round trip through a real file
// - exportDatabaseToJson / importDatabaseFromJson round trip, including
//   that import only touches tables that already exist in the target
//   Database (schema must be created ahead of import)

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

namespace {

Vector<ColumnDef> usersSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

std::string tempPath(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path();
    return (dir / ("minidb_serializer_" + label + ".json")).string();
}

} // namespace

// Verifies exportTableToJson()/importTableFromJson() (in-memory string,
// no file I/O) round trip preserves every record's id and fields.
TEST(SerializerIntegration, TableJsonStringRoundTrip) {
    Table original("users", 1, usersSchema());
    for (RecordID i = 1; i <= 3; ++i) {
        Record r(i);
        (void)r.setField("name", Json("user_" + std::to_string(i)));
        ASSERT_EQ(original.insertRecord(r), Status::OK);
    }

    std::string json = Serializer::exportTableToJson(original);

    Table restored("users", 1, usersSchema());
    ASSERT_EQ(Serializer::importTableFromJson(restored, json), Status::OK);

    ASSERT_EQ(restored.recordCount(), 3);
    Record out;
    ASSERT_EQ(restored.getRecord(2, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "user_2");
}

// Verifies exportTableToFile()/importTableFromFile() round trip through a
// real file on disk.
TEST(SerializerIntegration, TableFileRoundTrip) {
    std::string path = tempPath("table_file");
    std::filesystem::remove(path);

    Table original("users", 1, usersSchema());
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    ASSERT_EQ(original.insertRecord(r), Status::OK);

    ASSERT_EQ(Serializer::exportTableToFile(original, path), Status::OK);

    Table restored("users", 1, usersSchema());
    ASSERT_EQ(Serializer::importTableFromFile(restored, path), Status::OK);

    ASSERT_EQ(restored.recordCount(), 1);
    Record out;
    ASSERT_EQ(restored.getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");

    std::filesystem::remove(path);
}

// Verifies importTableFromFile on a nonexistent file returns a non-OK
// status rather than crashing.
TEST(SerializerIntegration, TableImportMissingFileReturnsError) {
    Table t("users", 1, usersSchema());
    EXPECT_NE(Serializer::importTableFromFile(t, tempPath("does_not_exist")), Status::OK);
}

// Verifies exportDatabaseToJson()/importDatabaseFromJson() round trip
// preserves records for every table that exists in the target Database.
TEST(SerializerIntegration, DatabaseJsonRoundTrip) {
    std::string path = tempPath("database_json");
    std::filesystem::remove(path);

    {
        Database db("shop");
        ASSERT_EQ(db.createTable("users", usersSchema()), Status::OK);
        Record r(1);
        (void)r.setField("name", Json("Ada"));
        ASSERT_EQ(db.getTable("users")->insertRecord(r), Status::OK);

        ASSERT_EQ(Serializer::exportDatabaseToJson(db, path), Status::OK);
    }

    // Import target must already have the table created (with a matching
    // schema) -- importDatabaseFromJson only populates tables that already
    // exist, it does not create them.
    Database restored("shop2");
    ASSERT_EQ(restored.createTable("users", usersSchema()), Status::OK);
    ASSERT_EQ(Serializer::importDatabaseFromJson(restored, path), Status::OK);

    ASSERT_EQ(restored.getTable("users")->recordCount(), 1);
    Record out;
    ASSERT_EQ(restored.getTable("users")->getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");

    std::filesystem::remove(path);
}

// Verifies importDatabaseFromJson skips tables present in the export that
// don't exist in the target Database, rather than failing the whole
// import.
TEST(SerializerIntegration, DatabaseImportSkipsUnknownTables) {
    std::string path = tempPath("database_skip_unknown");
    std::filesystem::remove(path);

    {
        Database db("shop");
        ASSERT_EQ(db.createTable("users", usersSchema()), Status::OK);
        ASSERT_EQ(db.createTable("orders", usersSchema()),
                  Status::OK); // reuse schema, name irrelevant here
        Record r(1);
        (void)r.setField("name", Json("Ada"));
        ASSERT_EQ(db.getTable("users")->insertRecord(r), Status::OK);

        ASSERT_EQ(Serializer::exportDatabaseToJson(db, path), Status::OK);
    }

    // Target only knows about "users", not "orders".
    Database restored("shop2");
    ASSERT_EQ(restored.createTable("users", usersSchema()), Status::OK);

    ASSERT_EQ(Serializer::importDatabaseFromJson(restored, path), Status::OK);
    EXPECT_EQ(restored.getTable("users")->recordCount(), 1);
    EXPECT_FALSE(restored.hasTable("orders"));

    std::filesystem::remove(path);
}
