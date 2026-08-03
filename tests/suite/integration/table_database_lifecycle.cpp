// Integration: Table/Database Lifecycle
// Exercises a realistic session across Database, Table, Page, and Record
// together: create tables, insert enough records to span multiple pages,
// update and delete records, drop a table and create a replacement,
// compact, then persist and reload -- checking the system behaves
// correctly as a whole through a sequence of mutations, not just in a
// single isolated operation.
//
// Covers:
// - insert/update/delete interleaved across multiple pages within one table
// - dropping a table and creating a new one mid-session does not disturb
//   surviving tables' data or identity
// - compact() after deletions reduces stored (non-deleted) footprint while
//   preserving all live records
// - the full post-mutation state survives a save/load round trip

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> usersSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"age", ColumnType::INT, false},
    };
}

std::string tempPath(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path();
    return (dir / ("minidb_lifecycle_" + label + ".json")).string();
}

Record makeUser(RecordID id, const std::string& name, int age) {
    Record r(id);
    (void)r.setField("name", Json(name));
    (void)r.setField("age", Json(age));
    return r;
}

} // namespace

// Verifies a table can be filled across multiple pages, then have records
// interleaved-updated and interleaved-deleted, with the survivors still
// correctly retrievable afterward.
static void insert_update_delete_across_multiple_pages() {
    Database db("app");
    CHK(db.createTable("users", usersSchema()) == Status::OK);
    Table* users = db.getTable("users");

    std::size_t count = static_cast<std::size_t>(DBConstants::MAX_RECORDS_PAGE) * 2 + 3;
    for (RecordID i = 0; i < count; ++i) {
        CHK(users->insertRecord(makeUser(i, "user_" + std::to_string(i), static_cast<int>(i))) ==
            Status::OK);
    }
    CHK(users->pageCount() >= 3);

    // Update every 10th record, delete every 7th.
    for (RecordID i = 0; i < count; ++i) {
        if (i % 10 == 0) {
            CHK(users->updateRecord(makeUser(i, "updated_" + std::to_string(i), -1)) == Status::OK);
        } else if (i % 7 == 0) {
            CHK(users->deleteRecord(i) == Status::OK);
        }
    }

    for (RecordID i = 0; i < count; ++i) {
        Record out;
        Status s = users->getRecord(i, out);
        if (i % 10 == 0) {
            CHK(s == Status::OK);
            CHK(out.getField("name").asString() == "updated_" + std::to_string(i));
        } else if (i % 7 == 0) {
            CHK(s == Status::NOT_FOUND);
        } else {
            CHK(s == Status::OK);
            CHK(out.getField("name").asString() == "user_" + std::to_string(i));
        }
    }
}

// Verifies dropping one table and creating a replacement mid-session does
// not disturb a surviving table's data or identity.
static void drop_and_recreate_does_not_disturb_survivors() {
    Database db("app");
    CHK(db.createTable("users", usersSchema()) == Status::OK);
    CHK(db.createTable("orders", usersSchema()) == Status::OK);
    CHK(db.createTable("logs", usersSchema()) == Status::OK);

    CHK(db.getTable("users")->insertRecord(makeUser(1, "Ada", 30)) == Status::OK);
    TableID usersId = db.getTable("users")->getID();

    CHK(db.dropTable("orders") == Status::OK);
    CHK(db.createTable("sessions", usersSchema()) == Status::OK);
    CHK(db.getTable("sessions")->insertRecord(makeUser(1, "sess_1", 0)) == Status::OK);

    // "users" must be entirely undisturbed.
    CHK(db.getTable("users")->getID() == usersId);
    Record out;
    CHK(db.getTable("users")->getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");

    CHK(db.hasTable("orders") == false);
    CHK(db.hasTable("logs") == true);
    CHK(db.hasTable("sessions") == true);
}

// Verifies compact() physically removes soft-deleted records while
// preserving all live records. Before compact(), recordCount() reflects
// both live and deleted records because deleted rows still occupy page
// slots. After compact(), only live records remain.
static void compact_after_deletions_preserves_live_records() {
    Database db("app");
    CHK(db.createTable("users", usersSchema()) == Status::OK);
    Table* users = db.getTable("users");

    for (RecordID i = 0; i < 20; ++i) {
        CHK(users->insertRecord(makeUser(i, "user_" + std::to_string(i), static_cast<int>(i))) ==
            Status::OK);
    }
    for (RecordID i = 0; i < 20; i += 2) {
        CHK(users->deleteRecord(i) == Status::OK);
    }
    CHK(users->recordCount() == 20);

    CHK(db.compact() == Status::OK);

    CHK(users->recordCount() == 10);
    for (RecordID i = 1; i < 20; i += 2) {
        Record out;
        CHK(users->getRecord(i, out) == Status::OK);
    }
    for (RecordID i = 0; i < 20; i += 2) {
        Record out;
        CHK(users->getRecord(i, out) == Status::NOT_FOUND);
    }
}

// Verifies the full post-mutation state (multi-page inserts, updates,
// deletes, a dropped table, a newly-created table) survives a save/load
// round trip intact.
static void full_session_survives_save_load() {
    std::string path = tempPath("full_session");
    std::filesystem::remove(path);

    {
        Database db("app");
        CHK(db.createTable("users", usersSchema()) == Status::OK);
        CHK(db.createTable("orders", usersSchema()) == Status::OK);

        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 5; ++i) {
            CHK(db.getTable("users")->insertRecord(
                    makeUser(i, "user_" + std::to_string(i), static_cast<int>(i))) == Status::OK);
        }
        CHK(db.getTable("users")->deleteRecord(3) == Status::OK);
        CHK(db.getTable("users")->updateRecord(makeUser(5, "updated_5", -1)) == Status::OK);

        CHK(db.dropTable("orders") == Status::OK);
        CHK(db.createTable("sessions", usersSchema()) == Status::OK);
        CHK(db.getTable("sessions")->insertRecord(makeUser(1, "sess_1", 0)) == Status::OK);

        CHK(db.save(path) == Status::OK);
    }

    Database restored("");
    CHK(restored.load(path) == Status::OK);

    CHK(restored.hasTable("orders") == false);
    CHK(restored.hasTable("users") == true);
    CHK(restored.hasTable("sessions") == true);

    Record out;
    CHK(restored.getTable("users")->getRecord(3, out) == Status::NOT_FOUND);
    CHK(restored.getTable("users")->getRecord(5, out) == Status::OK);
    CHK(out.getField("name").asString() == "updated_5");
    CHK(restored.getTable("users")->getRecord(0, out) == Status::OK);
    CHK(out.getField("name").asString() == "user_0");
    CHK(restored.getTable("sessions")->getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "sess_1");

    std::filesystem::remove(path);
}

// Executes all Table/Database lifecycle integration checks.
static void run_tests() {
    RUN(insert_update_delete_across_multiple_pages);
    RUN(drop_and_recreate_does_not_disturb_survivors);
    RUN(compact_after_deletions_preserves_live_records);
    RUN(full_session_survives_save_load);
}

REGISTER_TEST_SUITE();
