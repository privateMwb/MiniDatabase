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

#include <support/framework.h>

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
static void table_json_string_round_trip() {
    Table original("users", 1, usersSchema());
    for (RecordID i = 1; i <= 3; ++i) {
        Record r(i);
        (void)r.setField("name", Json("user_" + std::to_string(i)));
        CHK(original.insertRecord(r) == Status::OK);
    }

    std::string json = Serializer::exportTableToJson(original);

    Table restored("users", 1, usersSchema());
    CHK(Serializer::importTableFromJson(restored, json) == Status::OK);

    CHK(restored.recordCount() == 3);
    Record out;
    CHK(restored.getRecord(2, out) == Status::OK);
    CHK(out.getField("name").asString() == "user_2");
}

// Verifies exportTableToFile()/importTableFromFile() round trip through a
// real file on disk.
static void table_file_round_trip() {
    std::string path = tempPath("table_file");
    std::filesystem::remove(path);

    Table original("users", 1, usersSchema());
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    CHK(original.insertRecord(r) == Status::OK);

    CHK(Serializer::exportTableToFile(original, path) == Status::OK);

    Table restored("users", 1, usersSchema());
    CHK(Serializer::importTableFromFile(restored, path) == Status::OK);

    CHK(restored.recordCount() == 1);
    Record out;
    CHK(restored.getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");

    std::filesystem::remove(path);
}

// Verifies importTableFromFile on a nonexistent file returns a non-OK
// status rather than crashing.
static void table_import_missing_file_returns_error() {
    Table t("users", 1, usersSchema());
    CHK(Serializer::importTableFromFile(t, tempPath("does_not_exist")) != Status::OK);
}

// Verifies exportDatabaseToJson()/importDatabaseFromJson() round trip
// preserves records for every table that exists in the target Database.
static void database_json_round_trip() {
    std::string path = tempPath("database_json");
    std::filesystem::remove(path);

    {
        Database db("shop");
        CHK(db.createTable("users", usersSchema()) == Status::OK);
        Record r(1);
        (void)r.setField("name", Json("Ada"));
        CHK(db.getTable("users")->insertRecord(r) == Status::OK);

        CHK(Serializer::exportDatabaseToJson(db, path) == Status::OK);
    }

    // Import target must already have the table created (with a matching
    // schema) -- importDatabaseFromJson only populates tables that already
    // exist, it does not create them.
    Database restored("shop2");
    CHK(restored.createTable("users", usersSchema()) == Status::OK);
    CHK(Serializer::importDatabaseFromJson(restored, path) == Status::OK);

    CHK(restored.getTable("users")->recordCount() == 1);
    Record out;
    CHK(restored.getTable("users")->getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");

    std::filesystem::remove(path);
}

// Verifies importDatabaseFromJson skips tables present in the export that
// don't exist in the target Database, rather than failing the whole
// import.
static void database_import_skips_unknown_tables() {
    std::string path = tempPath("database_skip_unknown");
    std::filesystem::remove(path);

    {
        Database db("shop");
        CHK(db.createTable("users", usersSchema()) == Status::OK);
        CHK(db.createTable("orders", usersSchema()) ==
            Status::OK); // reuse schema, name irrelevant here
        Record r(1);
        (void)r.setField("name", Json("Ada"));
        CHK(db.getTable("users")->insertRecord(r) == Status::OK);

        CHK(Serializer::exportDatabaseToJson(db, path) == Status::OK);
    }

    // Target only knows about "users", not "orders".
    Database restored("shop2");
    CHK(restored.createTable("users", usersSchema()) == Status::OK);

    CHK(Serializer::importDatabaseFromJson(restored, path) == Status::OK);
    CHK(restored.getTable("users")->recordCount() == 1);
    CHK(restored.hasTable("orders") == false);

    std::filesystem::remove(path);
}

// Executes all Serializer integration checks.
static void run_tests() {
    RUN(table_json_string_round_trip);
    RUN(table_file_round_trip);
    RUN(table_import_missing_file_returns_error);
    RUN(database_json_round_trip);
    RUN(database_import_skips_unknown_tables);
}

REGISTER_TEST_SUITE();
