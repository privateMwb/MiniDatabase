// Full Application Workflow.
//
// Demonstrates:
// - Database + Table + QueryEngine + Serializer used together in one
//   realistic session: build a schema, load data, query it, export a
//   single table, then persist the whole database

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static void run_examples() {
    // A small shop database: two tables that will end up queried and
    // persisted together, the way a real application would use them.
    setTitle("Build The Schema");

    Database shop("shop");
    Vector<ColumnDef> orderSchema{
        ColumnDef{"item", ColumnType::STRING, false},
        ColumnDef{"amount", ColumnType::DOUBLE, false},
    };
    (void)shop.createTable("orders", orderSchema);
    Table* orders = shop.getTable("orders");
    std::cout << "tables in db : " << shop.tableCount() << "\n\n";

    // Load a handful of orders.
    setTitle("Insert Some Orders");

    struct Seed {
        RecordID id;
        const char* item;
        double amount;
    };
    for (const Seed& s : {Seed{1, "widget", 9.99}, Seed{2, "gadget", 24.50},
                          Seed{3, "widget", 9.99}, Seed{4, "gizmo", 100.0}}) {
        Record r(s.id);
        (void)r.setField("item", Json(s.item));
        (void)r.setField("amount", Json(s.amount));
        (void)orders->insertRecord(r);
    }
    std::cout << "orders inserted : " << orders->recordCount() << "\n\n";

    // Ask the QueryEngine a real question: total revenue from orders over
    // $10, sorted by amount.
    setTitle("Query It");

    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    FilterPredicate overTen{"amount", Op::GT, Json(10.0)};
    std::span<const FilterPredicate> preds(&overTen, 1);
    double total = engine.sum(*orders, "amount", preds);
    std::cout << "sum(amount) where amount > 10 : " << total << "\n\n";

    // Export just the orders table to its own file -- useful for sharing
    // one table's data without the rest of the database.
    setTitle("Export A Single Table");

    const std::string dir = "workflow_output";
    std::filesystem::create_directories(dir);
    Status s = Serializer::exportTableToFile(*orders, dir + "/orders.json");
    std::cout << "exportTableToFile status : " << static_cast<int>(s) << "\n\n";

    // Persist the entire database (schema + all tables) as one atomic
    // file, and reload it into a fresh Database to confirm it round-trips.
    setTitle("Save And Reload The Whole Database");

    s = shop.save(dir + "/shop.json");
    std::cout << "save status : " << static_cast<int>(s) << "\n";

    Database reloaded("shop_reloaded");
    s = reloaded.load(dir + "/shop.json");
    std::cout << "load status : " << static_cast<int>(s) << "\n";
    std::cout << "reloaded order count : " << reloaded.getTable("orders")->recordCount() << "\n";

    std::filesystem::remove_all(dir);
}

REGISTER_EXAMPLE_SUITE();