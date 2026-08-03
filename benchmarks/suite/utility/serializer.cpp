// Serializer Benchmark Suite
// Measures Serializer's file-based table export/import and
// database-wide JSON export/import.
//
// Covers:
// - exportTableToFile / importTableFromFile round trip
// - exportDatabaseToJson / importDatabaseFromJson round trip

#include <support/framework.h>

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
static void bench_export_table() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_serializer_export_table";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/orders.json";

    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 10);

    auto exportOnce = [&] { (void)Serializer::exportTableToFile(t, path); };
    BENCH_CUSTOM("Serializer exportTableToFile", exportOnce);

    fs::remove_all(dir);
}

// Measures importTableFromFile() reading a multi-page table back in.
// Uses a fresh destination table each iteration since insertRecord()
// rejects duplicate ids.
static void bench_import_table() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_serializer_import_table";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/orders.json";

    Table t = makeTable(DBConstants::MAX_RECORDS_PAGE * 10);
    (void)Serializer::exportTableToFile(t, path);

    auto importOnce = [&] {
        Table fresh("orders", 1, Vector<ColumnDef>{});
        (void)Serializer::importTableFromFile(fresh, path);
        doNotOptimize(fresh);
    };
    BENCH_CUSTOM("Serializer importTableFromFile", importOnce);

    fs::remove_all(dir);
}

// Measures exportDatabaseToJson() writing a multi-table database to disk.
static void bench_export_database() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_serializer_export_db";
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

    auto exportOnce = [&] { (void)Serializer::exportDatabaseToJson(db, path); };
    BENCH_CUSTOM("Serializer exportDatabaseToJson", exportOnce);

    fs::remove_all(dir);
}

// Measures importDatabaseFromJson() reading a multi-table database back
// in. The destination tables already exist (as importDatabaseFromJson
// requires); insertRecord() rejects duplicate ids, so the tables are
// recreated fresh each iteration.
static void bench_import_database() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_serializer_import_db";
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

    auto importOnce = [&] {
        Database db("shop");
        (void)db.createTable("orders", Vector<ColumnDef>{});
        (void)db.createTable("customers", Vector<ColumnDef>{});
        (void)Serializer::importDatabaseFromJson(db, path);
        doNotOptimize(db);
    };
    BENCH_CUSTOM("Serializer importDatabaseFromJson", importOnce);

    fs::remove_all(dir);
}

// Executes all Serializer benchmark cases.
static void run_benchmarks() {
    bench_export_table();
    std::cout << "\n";

    bench_import_table();
    std::cout << "\n";

    bench_export_database();
    std::cout << "\n";

    bench_import_database();
}

REGISTER_BENCH_SUITE();