// Concurrent Save/Load.
//
// Demonstrates:
// - Concurrency driving parallel save/load across multiple tables in one
//   Database
// - each table gets its own file, written/read on its own worker thread

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static void run_examples() {
    // A database with several independent tables -- the case Concurrency
    // is built for: each table's save/load is unrelated to the others', so
    // there's no reason to do them one at a time.
    setTitle("Build A Multi-Table Database");

    Database shop("shop");
    for (const char* name : {"orders", "customers", "inventory"}) {
        (void)shop.createTable(name, Vector<ColumnDef>{});
        Table* t = shop.getTable(name);
        for (RecordID i = 0; i < 20; ++i) {
            (void)t->insertRecord(Record(i));
        }
    }
    std::cout << "tables : " << shop.tableCount() << "\n\n";

    // Concurrency owns its own thread pool. saveAllTablesParallel() writes
    // one file per table (named "<baseFilename>_<tableName>.json"),
    // concurrently.
    setTitle("Save All Tables In Parallel");

    const std::string dir = "concurrent_output";
    std::filesystem::create_directories(dir);
    const std::string base = dir + "/shop";

    Concurrency conc;
    Status s = conc.saveAllTablesParallel(shop, base);
    std::cout << "saveAllTablesParallel status : " << static_cast<int>(s) << "\n\n";

    // Loading back in requires the destination tables to already exist
    // (loadAllTablesParallel populates existing tables, it doesn't create
    // them) -- so build a matching, empty shell first.
    setTitle("Load All Tables In Parallel");

    Database reloaded("shop_reloaded");
    for (const char* name : {"orders", "customers", "inventory"}) {
        (void)reloaded.createTable(name, Vector<ColumnDef>{});
    }

    s = conc.loadAllTablesParallel(reloaded, base);
    std::cout << "loadAllTablesParallel status : " << static_cast<int>(s) << "\n";
    for (const char* name : {"orders", "customers", "inventory"}) {
        std::cout << name << " record count : " << reloaded.getTable(name)->recordCount() << "\n";
    }

    std::filesystem::remove_all(dir);
}

REGISTER_EXAMPLE_SUITE();