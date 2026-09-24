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

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

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
TEST(TableDatabaseLifecycle, InsertUpdateDeleteAcrossMultiplePages) {
    Database db("app");
    ASSERT_EQ(db.createTable("users", usersSchema()), Status::OK);
    Table* users = db.getTable("users");

    std::size_t count = static_cast<std::size_t>(DBConstants::MAX_RECORDS_PAGE) * 2 + 3;
    for (RecordID i = 0; i < count; ++i) {
        ASSERT_EQ(
            users->insertRecord(makeUser(i, "user_" + std::to_string(i), static_cast<int>(i))),
            Status::OK);
    }
    ASSERT_GE(users->pageCount(), 3);

    // Update every 10th record, delete every 7th.
    for (RecordID i = 0; i < count; ++i) {
        if (i % 10 == 0) {
            ASSERT_EQ(users->updateRecord(makeUser(i, "updated_" + std::to_string(i), -1)),
                      Status::OK);
        } else if (i % 7 == 0) {
            ASSERT_EQ(users->deleteRecord(i), Status::OK);
        }
    }

    for (RecordID i = 0; i < count; ++i) {
        Record out;
        Status s = users->getRecord(i, out);
        if (i % 10 == 0) {
            EXPECT_EQ(s, Status::OK);
            EXPECT_EQ(out.getField("name").asString(), "updated_" + std::to_string(i));
        } else if (i % 7 == 0) {
            EXPECT_EQ(s, Status::NOT_FOUND);
        } else {
            EXPECT_EQ(s, Status::OK);
            EXPECT_EQ(out.getField("name").asString(), "user_" + std::to_string(i));
        }
    }
}

// Verifies dropping one table and creating a replacement mid-session does
// not disturb a surviving table's data or identity.
TEST(TableDatabaseLifecycle, DropAndRecreateDoesNotDisturbSurvivors) {
    Database db("app");
    ASSERT_EQ(db.createTable("users", usersSchema()), Status::OK);
    ASSERT_EQ(db.createTable("orders", usersSchema()), Status::OK);
    ASSERT_EQ(db.createTable("logs", usersSchema()), Status::OK);

    ASSERT_EQ(db.getTable("users")->insertRecord(makeUser(1, "Ada", 30)), Status::OK);
    TableID usersId = db.getTable("users")->getID();

    ASSERT_EQ(db.dropTable("orders"), Status::OK);
    ASSERT_EQ(db.createTable("sessions", usersSchema()), Status::OK);
    ASSERT_EQ(db.getTable("sessions")->insertRecord(makeUser(1, "sess_1", 0)), Status::OK);

    // "users" must be entirely undisturbed.
    EXPECT_EQ(db.getTable("users")->getID(), usersId);
    Record out;
    ASSERT_EQ(db.getTable("users")->getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "Ada");

    EXPECT_FALSE(db.hasTable("orders"));
    EXPECT_TRUE(db.hasTable("logs"));
    EXPECT_TRUE(db.hasTable("sessions"));
}

// Verifies compact() physically removes soft-deleted records while
// preserving all live records. Before compact(), recordCount() reflects
// both live and deleted records because deleted rows still occupy page
// slots. After compact(), only live records remain.
TEST(TableDatabaseLifecycle, CompactAfterDeletionsPreservesLiveRecords) {
    Database db("app");
    ASSERT_EQ(db.createTable("users", usersSchema()), Status::OK);
    Table* users = db.getTable("users");

    for (RecordID i = 0; i < 20; ++i) {
        ASSERT_EQ(
            users->insertRecord(makeUser(i, "user_" + std::to_string(i), static_cast<int>(i))),
            Status::OK);
    }
    for (RecordID i = 0; i < 20; i += 2) {
        ASSERT_EQ(users->deleteRecord(i), Status::OK);
    }
    ASSERT_EQ(users->recordCount(), 20);

    ASSERT_EQ(db.compact(), Status::OK);

    EXPECT_EQ(users->recordCount(), 10);
    for (RecordID i = 1; i < 20; i += 2) {
        Record out;
        EXPECT_EQ(users->getRecord(i, out), Status::OK);
    }
    for (RecordID i = 0; i < 20; i += 2) {
        Record out;
        EXPECT_EQ(users->getRecord(i, out), Status::NOT_FOUND);
    }
}

// Verifies the full post-mutation state (multi-page inserts, updates,
// deletes, a dropped table, a newly-created table) survives a save/load
// round trip intact.
TEST(TableDatabaseLifecycle, FullSessionSurvivesSaveLoad) {
    std::string path = tempPath("full_session");
    std::filesystem::remove(path);

    {
        Database db("app");
        ASSERT_EQ(db.createTable("users", usersSchema()), Status::OK);
        ASSERT_EQ(db.createTable("orders", usersSchema()), Status::OK);

        for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 5; ++i) {
            ASSERT_EQ(db.getTable("users")->insertRecord(
                          makeUser(i, "user_" + std::to_string(i), static_cast<int>(i))),
                      Status::OK);
        }
        ASSERT_EQ(db.getTable("users")->deleteRecord(3), Status::OK);
        ASSERT_EQ(db.getTable("users")->updateRecord(makeUser(5, "updated_5", -1)), Status::OK);

        ASSERT_EQ(db.dropTable("orders"), Status::OK);
        ASSERT_EQ(db.createTable("sessions", usersSchema()), Status::OK);
        ASSERT_EQ(db.getTable("sessions")->insertRecord(makeUser(1, "sess_1", 0)), Status::OK);

        ASSERT_EQ(db.save(path), Status::OK);
    }

    Database restored("");
    ASSERT_EQ(restored.load(path), Status::OK);

    EXPECT_FALSE(restored.hasTable("orders"));
    EXPECT_TRUE(restored.hasTable("users"));
    EXPECT_TRUE(restored.hasTable("sessions"));

    Record out;
    EXPECT_EQ(restored.getTable("users")->getRecord(3, out), Status::NOT_FOUND);
    ASSERT_EQ(restored.getTable("users")->getRecord(5, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "updated_5");
    ASSERT_EQ(restored.getTable("users")->getRecord(0, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "user_0");
    ASSERT_EQ(restored.getTable("sessions")->getRecord(1, out), Status::OK);
    EXPECT_EQ(out.getField("name").asString(), "sess_1");

    std::filesystem::remove(path);
}
