// Database Test Suite
//
// Comprehensive test suite for the Database implementation.
//
// Covers:
// - Table creation
// - Duplicate table name detection
// - Table lookup
// - Table dropping
// - Multi-table record counts
// - Database-wide compaction
// - Database save and load
//
// Each test validates behavior using assertions and prints a
// success message when completed.

#include "TestHelpers.h"
#include "../../include/Core/Database.h"

#include <cassert>
#include <iostream>
#include <cstdio>

// Create Table
// Verifies that a new table can be created and registered correctly.
static void test_createTable() {
    Database db("TestDB");

    Status s = db.createTable("employees", makeEmployeeSchema());

    assert(s == Status::OK);
    assert(db.tableCount() == 1);
    assert(db.hasTable("employees"));

    std::cout << "[PASS] createTable\n";
}


// Create Duplicate Table
// Ensures creating a table with a name that already exists is rejected.
static void test_createDuplicateTable() {
    Database db("TestDB");

    (void)db.createTable("employees", makeEmployeeSchema());
    Status s = db.createTable("employees", makeEmployeeSchema());

    assert(s == Status::TABLE_ALREADY_EXISTS);
    assert(db.tableCount() == 1);  // second create didn't sneak in

    std::cout << "[PASS] createDuplicateTable\n";
}

// Get Table
// Verifies that an existing table can be retrieved, and a missing one returns nullptr.
static void test_getTable() {
    Database db("TestDB");
    (void)db.createTable("employees", makeEmployeeSchema());

    Table* found = db.getTable("employees");
    assert(found != nullptr);
    assert(found->getName() == "employees");

    Table* missing = db.getTable("students");
    assert(missing == nullptr);

    std::cout << "[PASS] getTable\n";
}

// Drop Table
// Verifies that dropping a table removes it from the database and index.
static void test_dropTable() {
    Database db("TestDB");
    (void)db.createTable("employees", makeEmployeeSchema());

    Status s = db.dropTable("employees");
    assert(s == Status::OK);
    assert(db.tableCount() == 0);
    assert(!db.hasTable("employees"));

    std::cout << "[PASS] dropTable\n";
}

// Drop Missing Table
// Ensures dropping a non-existent table returns TABLE_NOT_FOUND.
static void test_dropTableNotFound() {
    Database db("TestDB");

    Status s = db.dropTable("ghosts");
    assert(s == Status::TABLE_NOT_FOUND);

    std::cout << "[PASS] dropTableNotFound\n";
}


// Multi-Table Record Count
// Verifies that recordCount sums correctly across multiple tables.
static void test_multiTableRecordCount() {
    Database db("TestDB");
    (void)db.createTable("employees", makeEmployeeSchema());
    (void)db.createTable("students", makeEmployeeSchema());

    Table* employees = db.getTable("employees");
    Table* students   = db.getTable("students");

    (void)employees->insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)employees->insertRecord(makeEmployeeRecord(2, "Bob", 60000));
    (void)students->insertRecord(makeEmployeeRecord(1, "Carol", 0));

    assert(db.recordCount() == 3);

    std::cout << "[PASS] multiTableRecordCount\n";
}

// Compact Database
// Verifies that compacting the database reclaims tombstoned records across all tables.
static void test_compactDatabase() {
    Database db("TestDB");
    (void)db.createTable("employees", makeEmployeeSchema());

    Table* employees = db.getTable("employees");
    (void)employees->insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)employees->insertRecord(makeEmployeeRecord(2, "Bob", 60000));
    (void)employees->deleteRecord(1);

    Status s = db.compact();
    assert(s == Status::OK);
    assert(!db.isDirty());
    assert(employees->recordCount() == 1);

    std::cout << "[PASS] compactDatabase\n";
}

// Save And Load Database
// Verifies that a database can be saved to disk and restored without data loss.
static void test_saveAndLoadDatabase() {
    Database original("TestDB");
    (void)original.createTable("employees", makeEmployeeSchema());

    Table* employees = original.getTable("employees");
    (void)employees->insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)employees->insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    const std::string path = "test_db_output.json";
    Status saveStatus = original.save(path);
    assert(saveStatus == Status::OK);

    Database restored("");
    Status loadStatus = restored.load(path);
    assert(loadStatus == Status::OK);

    assert(restored.getName() == "TestDB");
    assert(restored.tableCount() == 1);
    assert(restored.recordCount() == 2);

    Table* restoredEmployees = restored.getTable("employees");
    assert(restoredEmployees != nullptr);

    Record out;
    Status getStatus = restoredEmployees->getRecord(2, out);
    assert(getStatus == Status::OK);
    assert(out.getField("name").asString() == "Bob");

    std::remove(path.c_str());

    std::cout << "[PASS] saveAndLoadDatabase\n";
}

// Run Database Tests
// Executes all database unit tests and reports their results.
void run_database_tests() {
    std::cout << "Database Tests\n";
    test_createTable();
    test_createDuplicateTable();
    test_getTable();
    test_dropTable();
    test_dropTableNotFound();
    test_multiTableRecordCount();
    test_compactDatabase();
    test_saveAndLoadDatabase();
    std::cout << "\n\n";
}
