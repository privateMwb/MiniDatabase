// Table Test Suite
//
// Comprehensive test suite for the Table implementation.
//
// Covers:
// - Record insertion
// - Schema validation during insertion
// - Duplicate primary key detection
// - Record lookup
// - Record updates
// - Schema validation during updates
// - Record deletion
// - Missing record handling
// - Multi-page storage
// - Index rebuilding
// - Table serialization and deserialization
//
// Each test validates behavior using assertions and prints a
// success message when completed.

#include "TestHelpers.h"
#include <cassert>
#include <iostream>

// Default Construction
// Verifies that a newly created record has the correct default state.
static void test_insertRecord() {
    Table table = makeEmployeeTable();
    Record r = makeEmployeeRecord(1, "Alice", 50000);

    Status s = table.insertRecord(r);

    assert(s == Status::OK);
    assert(table.recordCount() == 1);
    std::cout << "[PASS] insertRecord\n";
}

// Field Access
// Verifies inserting, retrieving, and checking the existence of fields.
static void test_insertRecordInvalidSchema() {
    Table table = makeEmployeeTable();

    Record bad(2);
    (void)bad.setField("name", Json("Bob"));
    // missing required "salary"

    Status s = table.insertRecord(bad);
    assert(s == Status::INVALID_SCHEMA);

    std::cout << "[PASS] insertRecordInvalidSchema\n";
}

// Schema Validation
// Verifies records against a schema, including valid, missing-field, and invalid-type scenarios.
static void test_insertDuplicateKey() {
    Table table = makeEmployeeTable();
    Record r = makeEmployeeRecord(1, "Alice", 50000);

    (void)table.insertRecord(r);
    Status s = table.insertRecord(r);  // same ID again

    assert(s == Status::DUPLICATE_KEY);
    assert(table.recordCount() == 1);  // second insert didn't sneak in

    std::cout << "[PASS] insertDuplicateKey\n";
}

// Field Removal
// Verifies removing existing fields and handling missing fields.
static void test_getRecord() {
    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(5, "Carol", 70000));

    Record out;
    Status s = table.getRecord(5, out);

    assert(s == Status::OK);
    assert(out.getField("name").asString() == "Carol");

    Record missing;
    Status missingStatus = table.getRecord(999, missing);
    assert(missingStatus == Status::NOT_FOUND);

    std::cout << "[PASS] getRecord\n";
}

// Update Record
// Verifies that an existing record can be updated successfully.
static void test_updateRecord() {
    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(3, "Dave", 60000));

    Record updated = makeEmployeeRecord(3, "Dave", 75000);
    Status s = table.updateRecord(updated);

    assert(s == Status::OK);

    Record out;
    (void)table.getRecord(3, out);
    assert(out.getField("salary").asNumber() == 75000);

    std::cout << "[PASS] updateRecord\n";
}

// Update Record With Invalid Schema
// Ensures updates fail when the modified record violates the schema.
static void test_updateRecordInvalidSchema() {
    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(3, "Dave", 60000));

    Record bad(3);
    (void)bad.setField("name", Json("Dave"));
    (void)bad.setField("salary", Json("not a number"));

    Status s = table.updateRecord(bad);
    assert(s == Status::INVALID_TYPE);

    std::cout << "[PASS] updateRecordInvalidSchema\n";
}

// Delete Record
// Verifies that deleting a record removes it from the table and index.
static void test_deleteRecord() {
    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(8, "Eve", 80000));

    Status s = table.deleteRecord(8);
    assert(s == Status::OK);

    Record out;
    Status getStatus = table.getRecord(8, out);
    assert(getStatus == Status::NOT_FOUND);  // index entry removed immediately

    std::cout << "[PASS] deleteRecord\n";
}

// Delete Missing Record
// Ensures deleting a non-existent record returns NOT_FOUND.
static void test_deleteRecordNotFound() {
    Table table = makeEmployeeTable();

    Status s = table.deleteRecord(404);
    assert(s == Status::NOT_FOUND);

    std::cout << "[PASS] deleteRecordNotFound\n";
}

// Multi-Page Insert
// Verifies that records overflow correctly into additional pages.
static void test_multiPageInsert() {
    Table table = makeEmployeeTable();

    std::size_t totalToInsert = DBConstants::MAX_RECORDS_PAGE + 5;
    for (std::size_t i = 0; i < totalToInsert; i++) {
        Status s = table.insertRecord(makeEmployeeRecord(i, "Name", 1000));
        assert(s == Status::OK);
    }

    assert(table.recordCount() == totalToInsert);
    assert(table.pageCount() == 2);  // overflowed into a second page

    std::cout << "[PASS] multiPageInsert\n";
}

// Rebuild Index
// Verifies that the primary-key index can be reconstructed from stored records.
static void test_rebuildIndex() {
    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    Status s = table.rebuildIndex();
    assert(s == Status::OK);

    Record out;
    Status getStatus = table.getRecord(2, out);
    assert(getStatus == Status::OK);
    assert(out.getField("name").asString() == "Bob");

    std::cout << "[PASS] rebuildIndex\n";
}

// Serialize And Deserialize Table
// Verifies that a table can be serialized and restored without data loss.
static void test_serializeDeserializeTable() {
    Table original = makeEmployeeTable();
    (void)original.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)original.insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    std::string raw = original.serialize();

    Table restored("", DBConstants::INVALID_TABLE_ID, VectorPro<ColumnDef>{});
    Status s = restored.deserialize(raw);

    assert(s == Status::OK);
    assert(restored.getName() == "employees");
    assert(restored.recordCount() == 2);

    Record out;
    Status getStatus = restored.getRecord(1, out);
    assert(getStatus == Status::OK);
    assert(out.getField("name").asString() == "Alice");

    std::cout << "[PASS] test_serializeDeserializeTable\n";
}

// Run Table Tests
// Executes all table unit tests and reports their results.
void run_table_tests() {
    std::cout << "Table Tests\n";
    test_insertRecord();
    test_insertRecordInvalidSchema();
    test_insertDuplicateKey();
    test_getRecord();
    test_updateRecord();
    test_updateRecordInvalidSchema();
    test_deleteRecord();
    test_deleteRecordNotFound();
    test_multiPageInsert();
    test_rebuildIndex();
    test_serializeDeserializeTable();
    std::cout << "\n\n";
}
