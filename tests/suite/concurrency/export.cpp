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

#include <support/framework.h>

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
static void export_writes_one_file_per_table() {
    const std::string dir = tempDir("export");

    Database db("shop");
    CHK(db.createTable("orders", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("customers", Vector<ColumnDef>{}) == Status::OK);

    for (RecordID i = 0; i < 4; ++i) {
        CHK(db.getTable("orders")->insertRecord(Record(i)) == Status::OK);
    }
    for (RecordID i = 0; i < 2; ++i) {
        CHK(db.getTable("customers")->insertRecord(Record(i)) == Status::OK);
    }

    Concurrency conc;
    CHK(conc.exportAllTablesParallel(db, dir) == Status::OK);

    CHK(std::filesystem::exists(dir + "/orders.json"));
    CHK(std::filesystem::exists(dir + "/customers.json"));

    Table freshOrders("orders", 1, Vector<ColumnDef>{});
    CHK(Serializer::importTableFromFile(freshOrders, dir + "/orders.json") == Status::OK);
    CHK(freshOrders.recordCount() == 4);

    Table freshCustomers("customers", 2, Vector<ColumnDef>{});
    CHK(Serializer::importTableFromFile(freshCustomers, dir + "/customers.json") == Status::OK);
    CHK(freshCustomers.recordCount() == 2);
}

// Verifies soft-deleted records do not appear in the exported file, even
// when the export runs through the parallel path (not just the direct
// Serializer call).
static void export_excludes_deleted_records() {
    const std::string dir = tempDir("export_deleted");

    Database db("shop");
    CHK(db.createTable("orders", Vector<ColumnDef>{}) == Status::OK);
    for (RecordID i = 0; i < 5; ++i) {
        CHK(db.getTable("orders")->insertRecord(Record(i)) == Status::OK);
    }
    CHK(db.getTable("orders")->deleteRecord(2) == Status::OK);
    CHK(db.getTable("orders")->deleteRecord(4) == Status::OK);

    Concurrency conc;
    CHK(conc.exportAllTablesParallel(db, dir) == Status::OK);

    Table fresh("orders", 1, Vector<ColumnDef>{});
    CHK(Serializer::importTableFromFile(fresh, dir + "/orders.json") == Status::OK);
    CHK(fresh.recordCount() == 3);

    Record out;
    CHK(fresh.getRecord(0, out) == Status::OK);
    CHK(fresh.getRecord(1, out) == Status::OK);
    CHK(fresh.getRecord(2, out) == Status::NOT_FOUND);
    CHK(fresh.getRecord(3, out) == Status::OK);
    CHK(fresh.getRecord(4, out) == Status::NOT_FOUND);
}

// Verifies export correctness at a scale (10 tables) exceeding the default
// thread pool size, confirming every table's file is written with the
// right content, not just the first few to finish.
static void export_handles_more_tables_than_threads() {
    const std::string dir = tempDir("export_many");
    constexpr int kTableCount = 10;

    Database db("shop");
    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        CHK(db.createTable(name, Vector<ColumnDef>{}) == Status::OK);
        for (RecordID i = 0; i < static_cast<RecordID>(t + 1); ++i) {
            CHK(db.getTable(name)->insertRecord(Record(i)) == Status::OK);
        }
    }

    Concurrency conc;
    CHK(conc.exportAllTablesParallel(db, dir) == Status::OK);

    for (int t = 0; t < kTableCount; ++t) {
        std::string name = "t" + std::to_string(t);
        Table fresh(name, 1, Vector<ColumnDef>{});
        CHK(Serializer::importTableFromFile(fresh, dir + "/" + name + ".json") == Status::OK);
        CHK(fresh.recordCount() == static_cast<std::size_t>(t + 1));
    }
}

// Verifies exporting a table with zero records produces a valid,
// literally-empty JSON array file, not an empty file or malformed output.
static void export_empty_table() {
    const std::string dir = tempDir("export_empty_table");

    Database db("shop");
    CHK(db.createTable("empty_table", Vector<ColumnDef>{}) == Status::OK);

    Concurrency conc;
    CHK(conc.exportAllTablesParallel(db, dir) == Status::OK);

    std::string content;
    CHK(MiniDB::Common::FileIO::readFile(dir + "/empty_table.json", content) == Status::OK);
    CHK(content == "[]");
}

static void run_tests() {
    RUN(export_writes_one_file_per_table);
    RUN(export_excludes_deleted_records);
    RUN(export_handles_more_tables_than_threads);
    RUN(export_empty_table);
}

REGISTER_TEST_SUITE();