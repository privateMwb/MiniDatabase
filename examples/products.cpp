// Products Example
//
// Demonstrates a realistic "inventory" use case for MiniDatabase:
// managing two related product tables with low-stock filtering,
// price sorting, and parallel export via Concurrency.
//
// Covers:
// - Creating a database with multiple tables
// - Inserting multiple records per table
// - Filtering for low stock
// - Sorting by price
// - Parallel export across tables using PulseThreadPool
// - Thread pool introspection

#include "../include/Core/Database.h"
#include "../include/Engine/QueryEngine.h"
#include "../include/Engine/Concurrency.h"
#include "../libs/CustomAllocators/ArenaAllocator/ArenaAllocator.h"
#include "../libs/VectorPro/VectorPro.h"

#include <iostream>
#include <string>

// Builds the products table schema (shared by both tables).
static VectorPro<ColumnDef> makeProductSchema() {
    VectorPro<ColumnDef> schema;

    schema.push_back({ "name",  ColumnType::STRING, false });
    schema.push_back({ "price", ColumnType::DOUBLE, false });
    schema.push_back({ "stock", ColumnType::INT,    false });

    return schema;
}

// Builds a single product record.
static Record makeProduct(
    RecordID            id,
    const std::string&  name,
    double              price,
    int                 stock)
{
    Json fields(Json::ObjectType{});

    fields["name"]  = name;
    fields["price"] = price;
    fields["stock"] = stock;

    return Record(id, fields);
}

// Prints a single record's relevant fields in a readable line.
static void printProduct(const Record& r) {
    std::cout << "[" << r.getID() << "] "
               << r.getField("name").asString() << " - "
               << "$" << r.getField("price").asNumber() << " - "
               << r.getField("stock").asNumber() << " in stock\n";
}

int main() {
    std::cout << "=== MiniDatabase: Products Example ===\n\n";

    // 1. Create database with two tables: electronics and accessories
    Database db("ProductsDB");

    Status createElectronicsStatus = db.createTable("electronics", makeProductSchema());
    if (createElectronicsStatus != Status::OK) {
        std::cout << "Failed to create electronics table.\n";
        return 1;
    }
    std::cout << "Created electronics table.\n";

    Status createAccessoriesStatus = db.createTable("accessories", makeProductSchema());
    if (createAccessoriesStatus != Status::OK) {
        std::cout << "Failed to create accessories table.\n";
        return 1;
    }
    std::cout << "Created accessories table.\n\n";

    // 2. Insert the electronics records
    Table* electronics = db.getTable("electronics");
    if (!electronics) {
        std::cout << "Failed to retrieve electronics table.\n";
        return 1;
    }

    (void)electronics->insertRecord(makeProduct(1, "Laptop",  1200, 5));
    (void)electronics->insertRecord(makeProduct(2, "Monitor", 300,  12));
    (void)electronics->insertRecord(makeProduct(3, "Webcam",  60,   2));

    std::cout << "Inserted " << electronics->recordCount() << " electronics.\n";

    // 3. Insert the accessories records
    Table* accessories = db.getTable("accessories");
    if (!accessories) {
        std::cout << "Failed to retrieve accessories table.\n";
        return 1;
    }

    (void)accessories->insertRecord(makeProduct(1, "Mouse",     25, 80));
    (void)accessories->insertRecord(makeProduct(2, "Keyboard",  45, 3));
    (void)accessories->insertRecord(makeProduct(3, "USB Cable", 8,  200));

    std::cout << "Inserted " << accessories->recordCount() << " accessories.\n\n";


    // 4. Filter electronics for low stock (stock < 10)
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    FilterPredicate lowStockFilter{"stock", Op::LT, Json(10)};
    VectorPro<FilterPredicate*> stockPredicate;
    stockPredicate.push_back(&lowStockFilter);

    QueryResult lowStock = qe.select(*electronics, stockPredicate);

    std::cout << "Electronics with less than 10 in stock:\n";
    for (const Record& r : lowStock.records) {
        printProduct(r);
    }
    std::cout << "\n";


    // 5. Sort accessories by price ascending
    SortCondition priceSort{"price", SortOrder::ASC};
    VectorPro<FilterPredicate*> noPredicates;

    QueryResult byPrice = qe.select(*accessories, noPredicates, &priceSort);

    std::cout << "Accessories sorted by price (lowest first):\n";
    for (const Record& r : byPrice.records) {
        printProduct(r);
    }
    std::cout << "\n";


    // 6. Export both tables in parallel using Concurrency
    Concurrency       concurrency;
    const std::string outputDirectory = "examples/exported";

    Status exportStatus = concurrency.exportAllTablesParallel(db, outputDirectory);
    if (exportStatus != Status::OK) {
        std::cout << "Failed to export tables using Concurrency.\n";
        return 1;
    }

    std::cout << "Successfully exported both tables using Concurrency.\n\n";

    // 7. Print Concurrency introspection
    std::cout << "Thread Count: " << concurrency.threadCount() << "\n";
    std::cout << "Active Tasks: " << concurrency.activeTasks() << "\n";
    std::cout << "Queued Tasks: " << concurrency.queuedTasks() << "\n\n";

    return 0;
}
