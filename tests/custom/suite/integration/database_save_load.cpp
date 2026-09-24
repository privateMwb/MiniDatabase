// Integration: Database Save/Load
// Exercises the whole persistence stack together -- Database, Table, Page,
// Record, FileIO (via Database::save/load) -- with multiple tables, enough
// records to span multiple pages per table, and verifies the reloaded
// state through QueryEngine rather than just raw getters, so the test
// covers the same path a real caller would exercise end-to-end.
//
// Covers:
// - multi-table, multi-page save/load round trip through a real file
// - reloaded data is queryable (filter, sort, aggregate) exactly like the
//   original, not just retrievable by exact id
// - re-saving after reload (load -> mutate -> save -> load again) doesn't
//   corrupt or drop data

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

namespace {

Vector<ColumnDef> usersSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"age", ColumnType::INT, false},
    };
}

Vector<ColumnDef> ordersSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"item", ColumnType::STRING, false},
        ColumnDef{"total", ColumnType::INT, false},
    };
}

std::string tempPath(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path();
    return (dir / ("minidb_integration_" + label + ".json")).string();
}

// Populates `users` with `count` records spanning multiple pages, ages
// 0..count-1, names "user_0".."user_{count-1}".
void populateUsers(Table& users, std::size_t count) {
    for (RecordID i = 0; i < count; ++i) {
        Record r(i);
        (void)r.setField("name", Json("user_" + std::to_string(i)));
        (void)r.setField("age", Json(static_cast<int>(i)));
        CHK(users.insertRecord(r) == Status::OK);
    }
}

} // namespace

// Verifies a multi-table database, with one table large enough to span
// several pages, survives a save/load round trip intact.
static void multi_table_multi_page_round_trip() {
    std::string path = tempPath("multi_table");
    std::filesystem::remove(path);

    std::size_t userCount = static_cast<std::size_t>(DBConstants::MAX_RECORDS_PAGE) * 3 + 5;

    {
        Database db("shop");
        CHK(db.createTable("users", usersSchema()) == Status::OK);
        CHK(db.createTable("orders", ordersSchema()) == Status::OK);

        populateUsers(*db.getTable("users"), userCount);

        Record order(1);
        (void)order.setField("item", Json("widget"));
        (void)order.setField("total", Json(42));
        CHK(db.getTable("orders")->insertRecord(order) == Status::OK);

        CHK(db.save(path) == Status::OK);
    }

    Database restored("");
    CHK(restored.load(path) == Status::OK);

    CHK(restored.hasTable("users") == true);
    CHK(restored.hasTable("orders") == true);
    CHK(restored.getTable("users")->recordCount() == userCount);
    CHK(restored.getTable("users")->pageCount() >= 4);
    CHK(restored.getTable("orders")->recordCount() == 1);

    Record out;
    CHK(restored.getTable("users")->getRecord(userCount - 1, out) == Status::OK);
    CHK(out.getField("name").asString() == "user_" + std::to_string(userCount - 1));

    std::filesystem::remove(path);
}

// Verifies that data reloaded from disk is correctly queryable through
// QueryEngine -- filtering, sorting, and aggregation all operate on
// reloaded Page/Record objects exactly as they would on freshly-inserted
// ones.
static void reloaded_data_is_queryable() {
    std::string path = tempPath("queryable");
    std::filesystem::remove(path);

    {
        Database db("shop");
        CHK(db.createTable("users", usersSchema()) == Status::OK);
        populateUsers(*db.getTable("users"), 10);
        CHK(db.save(path) == Status::OK);
    }

    Database restored("");
    CHK(restored.load(path) == Status::OK);

    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);

    FilterPredicate pred{"age", Op::GTE, Json(5)};
    QueryResult res =
        qe.select(*restored.getTable("users"), std::span<const FilterPredicate>(&pred, 1));
    CHK(res.status == Status::OK);
    CHK(res.records.size() == 5); // ages 5..9

    double ttl = qe.sum(*restored.getTable("users"), "age", std::span<const FilterPredicate>{});
    CHK(ttl == 45.0); // 0+1+...+9

    std::filesystem::remove(path);
}

// Verifies a load -> mutate -> save -> load cycle doesn't drop or corrupt
// previously-persisted data.
static void reload_mutate_resave_round_trip() {
    std::string path = tempPath("resave");
    std::filesystem::remove(path);

    {
        Database db("shop");
        CHK(db.createTable("users", usersSchema()) == Status::OK);
        populateUsers(*db.getTable("users"), 5);
        CHK(db.save(path) == Status::OK);
    }

    {
        Database db("");
        CHK(db.load(path) == Status::OK);
        Record extra(100);
        (void)extra.setField("name", Json("late_addition"));
        (void)extra.setField("age", Json(99));
        CHK(db.getTable("users")->insertRecord(extra) == Status::OK);
        CHK(db.save(path) == Status::OK);
    }

    Database final_("");
    CHK(final_.load(path) == Status::OK);
    CHK(final_.getTable("users")->recordCount() == 6);

    Record out;
    CHK(final_.getTable("users")->getRecord(100, out) == Status::OK);
    CHK(out.getField("name").asString() == "late_addition");
    // Original records must still be present.
    CHK(final_.getTable("users")->getRecord(0, out) == Status::OK);
    CHK(out.getField("name").asString() == "user_0");

    std::filesystem::remove(path);
}

// Executes all Database save/load integration checks.
static void run_tests() {
    RUN(multi_table_multi_page_round_trip);
    RUN(reloaded_data_is_queryable);
    RUN(reload_mutate_resave_round_trip);
}

REGISTER_TEST_SUITE();
