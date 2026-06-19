// Serializer Test Suite
//
// Comprehensive test suite for the Serializer implementation.
//
// Covers:
// - Exporting a table to a JSON string
// - Importing a table from a JSON string
// - Round-tripping record IDs through export and import
// - Exporting and importing a table via file
// - Exporting and importing a full database via file
//
// Each test validates behavior using assertions and prints a
// success message when completed.

#include "TestHelpers.h"
#include "../include/Engine/Serializer.h"

#include <cassert>
#include <iostream>
#include <cstdio>

// Export Table To Json
// Verifies that exporting a table produces a clean JSON array of records.
static void test_exportTableToJson() {
    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    std::string json = Serializer::exportTableToJson(table);
    Json parsed = Json::parse(json);

    assert(!parsed.isNull());
    assert(parsed.isArray());
    assert(parsed.asArray().size() == 2);

    std::cout << "[PASS] exportTableToJson\n";
}

// Import Table From Json
// Verifies that importing a JSON array correctly inserts records into a table.
static void test_importTableFromJson() {
    Table table = makeEmployeeTable();

    std::string json = R"([{"name":"Alice","salary":50000},{"name":"Bob","salary":60000}])";
    Status s = Serializer::importTableFromJson(table, json);

    assert(s == Status::OK);
    assert(table.recordCount() == 2);

    std::cout << "[PASS] importTableFromJson\n";
}

// Export Then Import Preserves Id
// Verifies that a record's original ID survives a full export/import round trip.
static void test_exportImportPreservesId() {
    Table original = makeEmployeeTable();
    (void)original.insertRecord(makeEmployeeRecord(42, "Carol", 70000));

    std::string json = Serializer::exportTableToJson(original);

    Table imported = makeEmployeeTable();
    Status s = Serializer::importTableFromJson(imported, json);
    assert(s == Status::OK);

    Record out;
    Status getStatus = imported.getRecord(42, out);
    assert(getStatus == Status::OK);
    assert(out.getField("name").asString() == "Carol");

    // Confirm "id" did not leak into the actual data fields
    assert(!out.hasField("id"));

    std::cout << "[PASS] exportImportPreservesId\n";
}

// Export And Import Table Via File
// Verifies that a table can be exported to a file and re-imported correctly.
static void test_exportImportTableViaFile() {
    Table original = makeEmployeeTable();
    (void)original.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)original.insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    const std::string path = "serializer_table_test.json";
    Status exportStatus = Serializer::exportTableToFile(original, path);
    assert(exportStatus == Status::OK);

    Table imported = makeEmployeeTable();
    Status importStatus = Serializer::importTableFromFile(imported, path);
    assert(importStatus == Status::OK);
    assert(imported.recordCount() == 2);

    std::remove(path.c_str());

    std::cout << "[PASS] exportImportTableViaFile\n";
}

// Export And Import Database Via File
// Verifies that a full database can be exported and re-imported across multiple tables.
static void test_exportImportDatabaseViaFile() {
    Database source("TestDB");
    (void)source.createTable("employees", makeEmployeeSchema());
    (void)source.createTable("students", makeEmployeeSchema());

    Table* employees = source.getTable("employees");
    Table* students   = source.getTable("students");
    (void)employees->insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)students->insertRecord(makeEmployeeRecord(1, "Dan", 0));

    const std::string path = "serializer_db_test.json";
    Status exportStatus = Serializer::exportDatabaseToJson(source, path);
    assert(exportStatus == Status::OK);

    // Import into a fresh database with matching empty tables
    Database target("TestDB");
    (void)target.createTable("employees", makeEmployeeSchema());
    (void)target.createTable("students", makeEmployeeSchema());

    Status importStatus = Serializer::importDatabaseFromJson(target, path);
    assert(importStatus == Status::OK);

    assert(target.recordCount() == 2);

    std::remove(path.c_str());

    std::cout << "[PASS] exportImportDatabaseViaFile\n";
}
// Run Serializer Tests
// Executes all serializer unit tests and reports their results.
void run_serializer_tests() {
    std::cout << "Serializer Tests\n";
    test_exportTableToJson();
    test_importTableFromJson();
    test_exportImportPreservesId();
    test_exportImportTableViaFile();
    test_exportImportDatabaseViaFile();
    std::cout << "\n\n";
}
