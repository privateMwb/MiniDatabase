// Serializer Benchmark Suite
// Measures Serializer's file-based table export/import and
// database-wide JSON export/import.
//
// Covers:
// - exportTableToFile / importTableFromFile round trip
// - exportDatabaseToJson / importDatabaseFromJson round trip

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

// Builds a table with n plain records (ids 0..n-1).
static Table makeTable(RecordID n) {
    Table t("orders", 1, Vector<ColumnDef>{});
    for (RecordID i = 0; i < n; ++i) {
        (void)t.insertRecord(Record(i));
    }
    return t;
}

// Measures exportTableToFile() writing a multi-page table to disk.
static void table_export(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "serializer_export_table";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/orders.json";

    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 10);

    for (auto _ : state) {
        (void)Serializer::exportTableToFile(t, path);
    }

    fs::remove_all(dir);
}
BENCHMARK(table_export);

// Measures importTableFromFile() reading a multi-page table back in.
// Uses a fresh destination table each iteration since insertRecord()
// rejects duplicate ids.
static void table_import(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "serializer_import_table";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/orders.json";

    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 10);
    (void)Serializer::exportTableToFile(t, path);

    for (auto _ : state) {
        Table fresh("orders", 1, Vector<ColumnDef>{});
        (void)Serializer::importTableFromFile(fresh, path);
        benchmark::DoNotOptimize(fresh);
    }

    fs::remove_all(dir);
}
BENCHMARK(table_import);

// Measures exportDatabaseToJson() writing a multi-table database to disk.
static void database_export(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "serializer_export_db";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/db.json";

    Database db("shop");
    (void)db.createTable("orders", Vector<ColumnDef>{});
    (void)db.createTable("customers", Vector<ColumnDef>{});
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
        (void)db.getTable("orders")->insertRecord(Record(i));
        (void)db.getTable("customers")->insertRecord(Record(i));
    }

    for (auto _ : state) {
        (void)Serializer::exportDatabaseToJson(db, path);
    }

    fs::remove_all(dir);
}
BENCHMARK(database_export);

// Measures importDatabaseFromJson() reading a multi-table database back
// in. The destination tables already exist (as importDatabaseFromJson
// requires); insertRecord() rejects duplicate ids, so the tables are
// recreated fresh each iteration.
static void database_import(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "serializer_import_db";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/db.json";

    Database seed("shop");
    (void)seed.createTable("orders", Vector<ColumnDef>{});
    (void)seed.createTable("customers", Vector<ColumnDef>{});
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE * 5; ++i) {
        (void)seed.getTable("orders")->insertRecord(Record(i));
        (void)seed.getTable("customers")->insertRecord(Record(i));
    }
    (void)Serializer::exportDatabaseToJson(seed, path);

    for (auto _ : state) {
        Database db("shop");
        (void)db.createTable("orders", Vector<ColumnDef>{});
        (void)db.createTable("customers", Vector<ColumnDef>{});
        (void)Serializer::importDatabaseFromJson(db, path);
        benchmark::DoNotOptimize(db);
    }

    fs::remove_all(dir);
}
BENCHMARK(database_import);