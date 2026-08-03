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

#include <support/framework.h>

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
static void create_table_basic() {
    Database db("app");

    CHK(db.createTable("users", makeSchema()) == Status::OK);
    CHK(db.hasTable("users") == true);

    Table* t = db.getTable("users");
    CHK(t != nullptr);
    CHK(t->getName() == "users");
}

// Verifies creating a table with a name already in use fails.
static void create_duplicate_table_fails() {
    Database db("app");

    CHK(db.createTable("users", makeSchema()) == Status::OK);
    CHK(db.createTable("users", makeSchema()) == Status::TABLE_ALREADY_EXISTS);
}

// Verifies dropTable removes the table (hasTable/getTable reflect it).
static void drop_table_removes_it() {
    Database db("app");
    CHK(db.createTable("users", makeSchema()) == Status::OK);

    CHK(db.dropTable("users") == Status::OK);
    CHK(db.hasTable("users") == false);
    CHK(db.getTable("users") == nullptr);
}

// Verifies dropTable on a missing table returns TABLE_NOT_FOUND.
static void drop_missing_table_fails() {
    Database db("app");
    CHK(db.dropTable("ghost") == Status::TABLE_NOT_FOUND);
}

// Verifies getTable on a missing table returns nullptr rather than a
// dangling or default-constructed pointer.
static void get_missing_table_returns_nullptr() {
    Database db("app");
    CHK(db.getTable("ghost") == nullptr);
}

// Regression: creating a table after dropping another must not reuse the
// dropped table's id -- previously nextTableID() returned tables.size(),
// which collided with a surviving table's id after a drop, causing
// getTable() to resolve to the wrong table.
static void table_id_no_collision_after_drop_and_create() {
    Database db("app");
    CHK(db.createTable("a", makeSchema()) == Status::OK);
    CHK(db.createTable("b", makeSchema()) == Status::OK);
    CHK(db.createTable("c", makeSchema()) == Status::OK);

    TableID idC = db.getTable("c")->getID();

    CHK(db.dropTable("b") == Status::OK);
    CHK(db.createTable("d", makeSchema()) == Status::OK);

    // "c" must still resolve to itself, not be aliased by "d".
    CHK(db.getTable("c")->getID() == idC);
    CHK(db.getTable("c")->getName() == "c");
    CHK(db.getTable("d")->getName() == "d");
    CHK(db.getTable("d")->getID() != idC);
}

// Verifies toJson()/fromJson() preserves every table and its records.
static void json_round_trip_preserves_data() {
    Database original("app");
    CHK(original.createTable("users", makeSchema()) == Status::OK);
    CHK(original.getTable("users")->insertRecord(makeRecord(1, "Ada")) == Status::OK);

    Json envelope = original.toJson();

    Database restored("");
    CHK(restored.fromJson(envelope) == Status::OK);

    CHK(restored.getName() == "app");
    CHK(restored.hasTable("users") == true);
    CHK(restored.recordCount() == 1);

    Record out;
    CHK(restored.getTable("users")->getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");
}

// Verifies a full save()/load() round trip through a real file.
static void save_load_round_trip() {
    std::string path = tempPath("save_load");
    std::filesystem::remove(path);

    {
        Database original("app");
        CHK(original.createTable("users", makeSchema()) == Status::OK);
        CHK(original.getTable("users")->insertRecord(makeRecord(1, "Ada")) == Status::OK);
        CHK(original.save(path) == Status::OK);
    }

    Database restored("");
    CHK(restored.load(path) == Status::OK);
    CHK(restored.hasTable("users") == true);

    Record out;
    CHK(restored.getTable("users")->getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");

    std::filesystem::remove(path);
}

// Regression: load() on a corrupt file must fail cleanly and must not
// leave the database in a partial state -- previously a mid-load failure
// left `tables` with only some tables loaded and the rest silently
// missing.
static void load_corrupt_file_leaves_database_untouched() {
    std::string goodPath = tempPath("load_atomicity_good");
    std::string corruptPath = tempPath("load_atomicity_corrupt");
    std::filesystem::remove(goodPath);
    std::filesystem::remove(corruptPath);

    Database db("app");
    CHK(db.createTable("users", makeSchema()) == Status::OK);
    CHK(db.getTable("users")->insertRecord(makeRecord(1, "Ada")) == Status::OK);
    CHK(db.save(goodPath) == Status::OK);

    {
        std::ofstream corrupt(corruptPath, std::ios::binary);
        corrupt << "{ this is not valid json";
    }

    Status s = db.load(corruptPath);
    CHK(s != Status::OK);

    // The database must still reflect its pre-load-attempt state.
    CHK(db.hasTable("users") == true);
    Record out;
    CHK(db.getTable("users")->getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");

    std::filesystem::remove(goodPath);
    std::filesystem::remove(corruptPath);
}

// Verifies load() on a nonexistent file returns a non-OK status rather
// than crashing.
static void load_missing_file_returns_error() {
    Database db("app");
    CHK(db.load(tempPath("does_not_exist_" + std::to_string(std::rand()))) != Status::OK);
}

// Verifies compact() clears the dirty flag across all tables.
static void compact_clears_dirty() {
    Database db("app");
    CHK(db.createTable("users", makeSchema()) == Status::OK);
    CHK(db.getTable("users")->insertRecord(makeRecord(1, "Ada")) == Status::OK);
    CHK(db.isDirty() == true);

    CHK(db.compact() == Status::OK);
    CHK(db.isDirty() == false);
}

// Executes all Database test cases.
static void run_tests() {
    RUN(create_table_basic);
    RUN(create_duplicate_table_fails);
    RUN(drop_table_removes_it);
    RUN(drop_missing_table_fails);
    RUN(get_missing_table_returns_nullptr);
    RUN(table_id_no_collision_after_drop_and_create);
    RUN(json_round_trip_preserves_data);
    RUN(save_load_round_trip);
    RUN(load_corrupt_file_leaves_database_untouched);
    RUN(load_missing_file_returns_error);
    RUN(compact_clears_dirty);
}

REGISTER_TEST_SUITE();
