// Concurrency Export Test Suite
// Verifies exportAllTablesParallel() correctly writes one JSON file per
// table into a target directory, that the exported content excludes
// soft-deleted records (Serializer::exportTableToJson's own contract), and
// that this holds at a scale exceeding the default thread pool size.
//
// Covers:
// - one "<name>.json" file per table, each round-tripping back to the
//   right record count via Serializer::importTableFromFile
// - soft-deleted records are excluded from the exported file, even
//   through the parallel path
// - correctness at a table count (10) exceeding the default pool size
// - a zero-record table exports as a literal empty JSON array, not an
//   empty or malformed file

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

// Verifies exportAllTablesParallel() writes "<outputDirectory>/<name>.json"
// for each table, and that each file round-trips back into a fresh table
// with the same record count via Serializer::importTableFromFile.
TEST(ConcurrencyExport, WritesOneFilePerTable) {
    const std::string dir = tempDir("export");

    Database db("shop");
    ASSERT_EQ(db.createTable("orders", Vector<ColumnDef>{}), Status::OK);
    ASSERT_EQ(db.createTable("customers", Vector<ColumnDef>{}), Status::OK);

    for (RecordID i = 0; i < 4; ++i) {
        ASSERT_EQ(db.getTable("orders")->insertRecord(Record(i)), Status::OK);
    }
    for (RecordID i = 0; i < 2; ++i) {
        ASSERT_EQ(db.getTable("customers")->insertRecord(Record(i)), Status::OK);
    }

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.exportAllTablesParallel(db, dir), Status::OK);

    ASSERT_TRUE(std::filesystem::exists(dir + "/orders.json"));
    ASSERT_TRUE(std::filesystem::exists(dir + "/customers.json"));

    Table freshOrders("orders", 1, Vector<ColumnDef>{});
    ASSERT_EQ(Serializer::importTableFromFile(freshOrders, dir + "/orders.json"), Status::OK);
    ASSERT_EQ(freshOrders.recordCount(), 4);

    Table freshCustomers("customers", 2, Vector<ColumnDef>{});
    ASSERT_EQ(Serializer::importTableFromFile(freshCustomers, dir + "/customers.json"), Status::OK);
    ASSERT_EQ(freshCustomers.recordCount(), 2);
}

// Verifies soft-deleted records do not appear in the exported file, even
// when the export runs through the parallel path (not just the direct
// Serializer call).
TEST(ConcurrencyExport, ExcludesDeletedRecords) {
    const std::string dir = tempDir("export_deleted");

    Database db("shop");
    ASSERT_EQ(db.createTable("orders", Vector<ColumnDef>{}), Status::OK);
    for (RecordID i = 0; i < 5; ++i) {
        ASSERT_EQ(db.getTable("orders")->insertRecord(Record(i)), Status::OK);
    }
    ASSERT_EQ(db.getTable("orders")->deleteRecord(2), Status::OK);
    ASSERT_EQ(db.getTable("orders")->deleteRecord(4), Status::OK);

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.exportAllTablesParallel(db, dir), Status::OK);

    Table fresh("orders", 1, Vector<ColumnDef>{});
    ASSERT_EQ(Serializer::importTableFromFile(fresh, dir + "/orders.json"), Status::OK);
    ASSERT_EQ(fresh.recordCount(), 3);

    Record out;
    EXPECT_EQ(fresh.getRecord(0, out), Status::OK);
    EXPECT_EQ(fresh.getRecord(1, out), Status::OK);
    EXPECT_EQ(fresh.getRecord(2, out), Status::NOT_FOUND);
    EXPECT_EQ(fresh.getRecord(3, out), Status::OK);
    EXPECT_EQ(fresh.getRecord(4, out), Status::NOT_FOUND);
}

// Verifies export correctness at a scale (10 tables) exceeding the default
// thread pool size, confirming every table's file is written with the
// right content, not just the first few to finish.
TEST(ConcurrencyExport, HandlesMoreTablesThanThreads) {
    const std::string dir = tempDir("export_many");
    constexpr int kTableCount = 10;

    Database db("shop");
    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        ASSERT_EQ(db.createTable(name, Vector<ColumnDef>{}), Status::OK);
        for (RecordID i = 0; i < static_cast<RecordID>(t + 1); ++i) {
            ASSERT_EQ(db.getTable(name)->insertRecord(Record(i)), Status::OK);
        }
    }

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.exportAllTablesParallel(db, dir), Status::OK);

    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        Table fresh(name, 1, Vector<ColumnDef>{});
        ASSERT_EQ(Serializer::importTableFromFile(fresh, dir + "/" + name + ".json"), Status::OK);
        EXPECT_EQ(fresh.recordCount(), static_cast<std::size_t>(t + 1));
    }
}

// Verifies exporting a table with zero records produces a valid,
// literally-empty JSON array file, not an empty file or malformed output.
TEST(ConcurrencyExport, EmptyTable) {
    const std::string dir = tempDir("export_empty_table");

    Database db("shop");
    ASSERT_EQ(db.createTable("empty_table", Vector<ColumnDef>{}), Status::OK);

    MiniDB::Engine::Concurrency conc;
    ASSERT_EQ(conc.exportAllTablesParallel(db, dir), Status::OK);

    std::string content;
    ASSERT_EQ(MiniDB::Common::FileIO::readFile(dir + "/empty_table.json", content), Status::OK);
    EXPECT_EQ(content, "[]");
}
