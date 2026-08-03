// Save and Load.
//
// Demonstrates:
// - persisting a Database to a real file with save()
// - loading it back into a fresh Database with load()
// - that save() is atomic: a real file on disk is either the old complete
//   version or the new complete version, never a partial write
// - that a failed load leaves the target Database completely untouched

#include <support/framework.h>

#include <filesystem>
#include <fstream>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static void run_examples() {
    std::string path =
        (std::filesystem::temp_directory_path() / "minidb_example_save_load.json").string();
    std::filesystem::remove(path);

    // Build a small database with one table and a couple of records.
    setTitle("Build and Save");

    Database db("shop");
    (void)db.createTable("orders", Vector<ColumnDef>{
                                       ColumnDef{"item", ColumnType::STRING, false},
                                   });

    Table* orders = db.getTable("orders");
    Record order1(1);
    (void)order1.setField("item", Json("widget"));
    (void)orders->insertRecord(order1);

    Record order2(2);
    (void)order2.setField("item", Json("gadget"));
    (void)orders->insertRecord(order2);

    Status s = db.save(path);
    std::cout << "save() status : " << static_cast<int>(s) << "\n";
    std::cout << "file exists   : " << std::filesystem::exists(path) << "\n\n";

    // Load into a completely fresh Database and confirm the data made it.
    setTitle("Load Into a Fresh Database");

    Database restored("");
    s = restored.load(path);
    std::cout << "load() status         : " << static_cast<int>(s) << "\n";
    std::cout << "restored name         : " << restored.getName() << "\n";
    std::cout << "restored record count : " << restored.getTable("orders")->recordCount() << "\n\n";

    // A failed load (here: a corrupt file) leaves the target Database
    // exactly as it was -- save()/load() are all-or-nothing, so a bad load
    // attempt can never leave you with half your data replaced.
    setTitle("A Failed Load Changes Nothing");

    std::string corruptPath =
        (std::filesystem::temp_directory_path() / "minidb_example_corrupt.json").string();
    {
        std::ofstream corrupt(corruptPath, std::ios::binary);
        corrupt << "{ not valid json";
    }

    std::cout << "restored table count before bad load : " << restored.tableCount() << "\n";
    s = restored.load(corruptPath);
    std::cout << "load(corrupt file) status            : " << static_cast<int>(s) << "\n";
    std::cout << "restored table count after bad load   : " << restored.tableCount() << "\n";
    std::cout << "restored data still intact            : "
              << restored.getTable("orders")->recordCount() << " record(s)\n";

    std::filesystem::remove(path);
    std::filesystem::remove(corruptPath);
}

REGISTER_EXAMPLE_SUITE();
