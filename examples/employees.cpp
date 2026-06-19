// Employees Example
//
// Demonstrates a realistic "business" use case for MiniDatabase:
// managing an employee roster with department-based filtering,
// salary aggregates, and full persistence to disk.
//
// Covers:
// - Creating a database and table
// - Inserting multiple records
// - Filtering by department
// - Sorting by salary
// - Aggregate queries (sum, avg)
// - Updating and deleting a record
// - Saving and loading the database

#include "../include/Core/Database.h"
#include "../include/Engine/QueryEngine.h"
#include "../libs/CustomAllocators/ArenaAllocator/ArenaAllocator.h"

#include <iostream>

// Builds the employees table schema.
static VectorPro<ColumnDef> makeEmployeeSchema() {
    VectorPro<ColumnDef> schema;
    schema.push_back({ "name",       ColumnType::STRING, false });
    schema.push_back({ "salary",     ColumnType::DOUBLE, false });
    schema.push_back({ "department", ColumnType::STRING, false });
    return schema;
}

// Builds a single employee record.
static Record makeEmployee(RecordID id, const std::string& name, double salary, const std::string& dept) {
    Json fields(Json::ObjectType{});
    fields["name"] = name;
    fields["salary"] = salary;
    fields["department"] = dept;
    return Record(id, fields);
}

// Prints a single record's relevant fields in a readable line.
static void printEmployee(const Record& r) {
    std::cout << "  [" << r.getID() << "] "
        << r.getField("name").asString()
        << " - $" << r.getField("salary").asNumber()
        << " (" << r.getField("department").asString() << ")\n";
}

int main() {
    std::cout << "=== MiniDatabase: Employees Example ===\n\n";

    // 1. Create database and table
    Database db("CompanyDB");
    Status createStatus = db.createTable("employees", makeEmployeeSchema());
    if (createStatus != Status::OK) {
        std::cout << "Failed to create table.\n";
        return 1;
    }

    Table* employees = db.getTable("employees");

    // 2. Insert records
    (void)employees->insertRecord(makeEmployee(1, "Alice",   85000, "Engineering"));
    (void)employees->insertRecord(makeEmployee(2, "Bob",     62000, "Sales"));
    (void)employees->insertRecord(makeEmployee(3, "Carol",   95000, "Engineering"));
    (void)employees->insertRecord(makeEmployee(4, "Dave",    58000, "Support"));
    (void)employees->insertRecord(makeEmployee(5, "Eve",     72000, "Sales"));

    std::cout << "Inserted " << employees->recordCount() << " employees.\n\n";


    // 3. Filter by department
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    FilterPredicate engineeringFilter{"department", Op::EQ, Json("Engineering")};
    VectorPro<FilterPredicate*> deptPredicates;
    deptPredicates.push_back(&engineeringFilter);

    QueryResult engineers = qe.select(*employees, deptPredicates);

    std::cout << "Engineering department:\n";
    for (const Record& r : engineers.records) {
        printEmployee(r);
    }
    std::cout << "\n";

    // 4. Sort by salary descending
    SortCondition salarySort{"salary", SortOrder::DESC};
    VectorPro<FilterPredicate*> noPredicates;

    QueryResult bySalary = qe.select(*employees, noPredicates, &salarySort);

    std::cout << "All employees, sorted by salary (highest first):\n";
    for (const Record& r : bySalary.records) {
        printEmployee(r);
    }
    std::cout << "\n";

    // 6. Update a record
    Status updateStatus = employees->updateRecord(makeEmployee(4, "Dave", 65000, "Support"));
    std::cout << "Dave's raise: " << (updateStatus == Status::OK ? "approved" : "failed") << "\n";

    // 7. Delete a record
    Status deleteStatus = employees->deleteRecord(2);
    std::cout << "Bob's departure: " << (deleteStatus == Status::OK ? "processed" : "failed") << "\n\n";

    // 8. Save and load
    const std::string path = "examples/exported/employees.json";
    Status saveStatus = db.save(path);
    std::cout << "Save to disk: " << (saveStatus == Status::OK ? "success" : "failed") << "\n";

    Database restored("");
    Status loadStatus = restored.load(path);
    std::cout << "Load from disk: " << (loadStatus == Status::OK ? "success" : "failed") << "\n\n";

    std::cout << "Restored database has " << restored.tableCount() << " table(s) "
              << "and " << restored.recordCount() << " record(s).\n";

    return 0;
}
