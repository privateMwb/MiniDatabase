// Concurrency Test Suite
//
// Comprehensive test suite for the Concurrency (PulseThreadPool integration) implementation.
//
// Covers:
// - Parallel save of all tables in a database
// - Parallel load of all tables in a database
// - Parallel rebuilding of all table indexes
// - Parallel export of all tables to JSON files
//
// Since task execution order is not deterministic, these tests assert
// on the final state after all parallel work has completed, rather
// than on timing or ordering.
//
// Each test validates behavior using assertions and prints a
// success message when completed.

#include "TestHelpers.h"
#include "../include/Engine/Concurrency.h"
#include "../include/Engine/Serializer.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

// Builds a database with two independent tables, each holding records.
// Used across every concurrency test so multiple tables can run in parallel
// without any risk of interfering with each other.
static Database makeMultiTableDatabase() {
    Database db("ConcurrencyTestDB");
    (void)db.createTable("employees", makeEmployeeSchema());
    (void)db.createTable("students", makeEmployeeSchema());

    Table* employees = db.getTable("employees");
    Table* students   = db.getTable("students");

    (void)employees->insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)employees->insertRecord(makeEmployeeRecord(2, "Bob", 60000));
    (void)students->insertRecord(makeEmployeeRecord(1, "Carol", 0));

    return db;
}

// Save All Tables Parallel
// Verifies that every table in a database is correctly written to its own file concurrently.
static void test_saveAllTablesParallel() {
    Database db = makeMultiTableDatabase();
    Concurrency concurrency;

    const std::string baseFilename = "concurrency_save_test";

    Status s = concurrency.saveAllTablesParallel(db, baseFilename);
    assert(s == Status::OK);

    // Confirm both files were actually written
    std::ifstream employeesFile(baseFilename + "_employees.json");
    std::ifstream studentsFile(baseFilename + "_students.json");

    assert(employeesFile.is_open());
    assert(studentsFile.is_open());

    employeesFile.close();
    studentsFile.close();

    std::remove((baseFilename + "_employees.json").c_str());
    std::remove((baseFilename + "_students.json").c_str());

    std::cout << "[PASS] saveAllTablesParallel\n";
}


// Load All Tables Parallel
// Verifies that every table's file is correctly read back concurrently into existing tables.
static void test_loadAllTablesParallel() {
    Database source = makeMultiTableDatabase();
    Concurrency concurrency;

    const std::string baseFilename = "concurrency_load_test";
    Status saveStatus = concurrency.saveAllTablesParallel(source, baseFilename);
    assert(saveStatus == Status::OK);

    // Fresh target database with matching empty tables
    Database target("ConcurrencyTestDB");
    (void)target.createTable("employees", makeEmployeeSchema());
    (void)target.createTable("students", makeEmployeeSchema());

    Status loadStatus = concurrency.loadAllTablesParallel(target, baseFilename);
    assert(loadStatus == Status::OK);

    assert(target.recordCount() == 3);

    std::remove((baseFilename + "_employees.json").c_str());
    std::remove((baseFilename + "_students.json").c_str());

    std::cout << "[PASS] loadAllTablesParallel\n";
}


// Rebuild All Indexes Parallel
// Verifies that every table's index is correctly rebuilt concurrently.
static void test_rebuildAllIndexesParallel() {
    Database db = makeMultiTableDatabase();
    Concurrency concurrency;

    Status s = concurrency.rebuildAllIndexesParallel(db);
    assert(s == Status::OK);

    Table* employees = db.getTable("employees");
    Table* students   = db.getTable("students");

    Record out;
    Status empStatus = employees->getRecord(2, out);
    assert(empStatus == Status::OK);
    assert(out.getField("name").asString() == "Bob");

    Status stuStatus = students->getRecord(1, out);
    assert(stuStatus == Status::OK);
    assert(out.getField("name").asString() == "Carol");

    std::cout << "[PASS] rebuildAllIndexesParallel\n";
}

// Export All Tables Parallel
// Verifies that every table is exported to its own JSON file concurrently.
static void test_exportAllTablesParallel() {
    Database db = makeMultiTableDatabase();
    Concurrency concurrency;

    const std::string outputDir = ".";

    Status s = concurrency.exportAllTablesParallel(db, outputDir);
    assert(s == Status::OK);

    std::ifstream employeesFile("./employees.json");
    std::ifstream studentsFile("./students.json");

    assert(employeesFile.is_open());
    assert(studentsFile.is_open());

    employeesFile.close();
    studentsFile.close();

    std::remove("./employees.json");
    std::remove("./students.json");

    std::cout << "[PASS] exportAllTablesParallel\n";
}

// Introspection
// Verifies that thread pool introspection methods return sane values.
static void test_introspection() {
    Concurrency concurrency(4);

    assert(concurrency.threadCount() == 4);
    assert(concurrency.activeTasks() == 0);   // no tasks running yet
    assert(concurrency.queuedTasks() == 0);   // nothing queued yet

    std::cout << "[PASS] introspection\n";
}

// Run Concurrency Tests
// Executes all concurrency unit tests and reports their results.
void run_concurrency_tests() {
    std::cout << "Concurrency Tests\n";
    test_saveAllTablesParallel();
    test_loadAllTablesParallel();
    test_rebuildAllIndexesParallel();
    test_exportAllTablesParallel();
    test_introspection();
    std::cout << "\n\n";
}
