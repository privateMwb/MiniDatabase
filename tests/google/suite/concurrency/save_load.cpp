// Concurrency Save/Load Test Suite
// Verifies saveAllTablesParallel()/loadAllTablesParallel() correctly write
// and restore every table in a Database concurrently via the shared
// runParallel() helper (Concurrency.cpp), including at a scale where the
// number of tables exceeds the default thread pool size.
//
// Covers:
// - a small multi-table round trip: each table gets its own
//   "<base>_<name>.json" file, restored correctly into a fresh Database
// - correctness at a table count (12) exceeding the default pool size,
//   including tables spanning multiple pages
// - loadAllTablesParallel() inserts additively (doesn't clear the target
//   table first), so a colliding id reports Status::DUPLICATE_KEY

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

namespace {
std::string tempDir(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path() / ("minidb_concurrency_" + label);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}
} // namespace

// Verifies a small multi-table database round-trips through
// saveAllTablesParallel() -> loadAllTablesParallel(): each table gets its
// own "<base>_<name>.json" file, and loading into a fresh Database with
// matching (empty) tables restores every record correctly.
TEST(ConcurrencySaveLoad, RoundTrip) {
    const std::string base = tempDir("save_load") + "/db";

    Vector<ColumnDef> orderSchema{ColumnDef{"amount", ColumnType::INT, false}};
    Vector<ColumnDef> customerSchema{ColumnDef{"name", ColumnType::STRING, false}};

    Database db("shop");
    ASSERT_EQ(db.createTable("orders", orderSchema), Status::OK);
    ASSERT_EQ(db.createTable("customers", customerSchema), Status::OK);
    ASSERT_EQ(db.createTable("logs", Vector<ColumnDef>{}), Status::OK);

    for (RecordID i = 0; i < 5; ++i) {
        Record r(i);
        ASSERT_EQ(r.setField("amount", Json(static_cast<int>(i * 10))), Status::OK);
        ASSERT_EQ(db.getTable("orders")->insertRecord(r), Status::OK);
    }
    for (RecordID i = 0; i < 3; ++i) {
        Record r(i);
        ASSERT_EQ(r.setField("name", Json("customer_" + std::to_string(i))), Status::OK);
        ASSERT_EQ(db.getTable("customers")->insertRecord(r), Status::OK);
    }

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.saveAllTablesParallel(db, base), Status::OK);

    ASSERT_TRUE(std::filesystem::exists(base + "_orders.json"));
    ASSERT_TRUE(std::filesystem::exists(base + "_customers.json"));
    ASSERT_TRUE(std::filesystem::exists(base + "_logs.json"));

    Database restored("shop_restored");
    ASSERT_EQ(restored.createTable("orders", orderSchema), Status::OK);
    ASSERT_EQ(restored.createTable("customers", customerSchema), Status::OK);
    ASSERT_EQ(restored.createTable("logs", Vector<ColumnDef>{}), Status::OK);

    ASSERT_EQ(conc.loadAllTablesParallel(restored, base), Status::OK);

    EXPECT_EQ(restored.getTable("orders")->recordCount(), 5);
    EXPECT_EQ(restored.getTable("customers")->recordCount(), 3);
    EXPECT_EQ(restored.getTable("logs")->recordCount(), 0);

    Record out;
    ASSERT_EQ(restored.getTable("orders")->getRecord(3, out), Status::OK);
    EXPECT_EQ(out.getField("amount").asNumber(), 30);
}

// Verifies correctness at a scale where the number of tables (12) exceeds
// the default thread pool size (DBConstants::THREAD_POOL_SIZE == 4), and
// where some tables span multiple pages -- stresses task
// queueing/collection, not just single-task correctness.
TEST(ConcurrencySaveLoad, ManyTables) {
    const std::string base = tempDir("save_load_many") + "/db";
    constexpr int kTableCount = 12;

    Database db("shop");
    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        ASSERT_EQ(db.createTable(name, Vector<ColumnDef>{}), Status::OK);

        // Every third table spans two pages.
        std::size_t n = (t % 3 == 0) ? DBConstants::MAX_RECORDS_PAGE + 10 : 5;
        for (RecordID i = 0; i < n; ++i) {
            ASSERT_EQ(db.getTable(name)->insertRecord(Record(i)), Status::OK);
        }
    }

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.saveAllTablesParallel(db, base), Status::OK);

    Database restored("shop_restored");
    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        ASSERT_EQ(restored.createTable(name, Vector<ColumnDef>{}), Status::OK);
    }

    ASSERT_EQ(conc.loadAllTablesParallel(restored, base), Status::OK);

    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        std::size_t expected = (t % 3 == 0) ? DBConstants::MAX_RECORDS_PAGE + 10 : 5;
        EXPECT_EQ(restored.getTable(name)->recordCount(), expected);
    }
}

// Verifies loadAllTablesParallel() inserts additively rather than
// replacing a table's contents: if the destination table already holds a
// record whose id collides with one in the loaded file, the load reports
// Status::DUPLICATE_KEY. (importTableFromFile never clears the table
// first -- worth pinning explicitly.)
TEST(ConcurrencySaveLoad, LoadDuplicateKey) {
    const std::string base = tempDir("save_load_duplicate") + "/db";

    Database source("shop");
    ASSERT_EQ(source.createTable("orders", Vector<ColumnDef>{}), Status::OK);
    for (RecordID i = 0; i < 3; ++i) {
        ASSERT_EQ(source.getTable("orders")->insertRecord(Record(i)), Status::OK);
    }

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.saveAllTablesParallel(source, base), Status::OK);

    Database target("shop_target");
    ASSERT_EQ(target.createTable("orders", Vector<ColumnDef>{}), Status::OK);
    ASSERT_EQ(target.getTable("orders")->insertRecord(Record(0)),
              Status::OK); // collides with file's id 0

    EXPECT_EQ(conc.loadAllTablesParallel(target, base), Status::DUPLICATE_KEY);
}
