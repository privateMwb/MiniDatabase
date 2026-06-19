// Record Test Suite
//
// Verifies the functionality of the Record class.
//
// Covers:
// - Default construction
// - Field insertion and retrieval
// - Schema validation
// - Field removal
// - Serialization and deserialization
// - Soft deletion
//
// Each test uses assertions to validate expected behavior and
// prints a success message upon completion.

#include "TestHelpers.h"
#include <cassert>
#include <iostream>

// Default Construction
// Verifies that a newly created record has the correct default state.
static void test_constructDefault() {
    Record r;
    assert(r.getID() == DBConstants::INVALID_RECORD_ID);
    assert(!r.isDeleted());
    std::cout << "[PASS] constructDefault\n";
}

// Field Access
// Verifies inserting, retrieving, and checking the existence of fields.
static void test_setAndGetField() {
    Record r(1);
    Status s = r.setField("name", Json("Alice"));

    assert(s == Status::OK);
    assert(r.hasField("name"));
    assert(r.getField("name").asString() == "Alice");
    std::cout << "[PASS] setAndGetField\n";
}

// Schema Validation
// Verifies records against a schema, including valid, missing-field, and invalid-type scenarios.
static void test_validateSchema() {
    VectorPro<ColumnDef> schema = makeEmployeeSchema();

    Record valid = makeEmployeeRecord(1, "Bob", 60000);
    assert(valid.validate(schema) == Status::OK);

    Record missingField(2);
    (void)missingField.setField("name", Json("Charlie"));
    // missing required "salary"
    assert(missingField.validate(schema) == Status::INVALID_SCHEMA);

    Record wrongType(3);
    (void)wrongType.setField("name", Json("Dana"));
    (void)wrongType.setField("salary", Json("not a number"));
    assert(wrongType.validate(schema) == Status::INVALID_TYPE);

    std::cout << "[PASS] validateSchema\n";
}

// Field Removal
// Verifies removing existing fields and handling missing fields.
static void test_removeField() {
    Record r(1);
    (void)r.setField("name", Json("Alice"));

    Status s = r.removeField("name");
    assert(s == Status::OK);
    assert(!r.hasField("name"));

    Status missing = r.removeField("name");
    assert(missing == Status::NOT_FOUND);
    std::cout << "[PASS] removeField\n";
}

// Serialization
// Verifies serializing a record and restoring it through deserialization.
static void test_serializeDeserialize() {
    Record original = makeEmployeeRecord(5, "Eve", 75000);
    std::string raw = original.serialize();

    Record restored;
    Status s = restored.deserialize(raw);

    assert(s == Status::OK);
    assert(restored.getID() == 5);
    assert(restored.getField("name").asString() == "Eve");
    assert(restored.getField("salary").asNumber() == 75000);

    std::cout << "[PASS] serializeDeserialize\n";
}

// Soft Deletion
// Verifies marking a record as deleted without destroying its data.
static void test_markDeleted() {
    Record r(1);
    assert(!r.isDeleted());

    r.markDeleted();
    assert(r.isDeleted());

    std::cout << "[PASS] markDeleted\n";
}

// Test Runner
// Executes every Record test and prints the overall test section.
void run_record_tests() {
    std::cout << "Record Tests\n";
    test_constructDefault();
    test_setAndGetField();
    test_removeField();
    test_validateSchema();
    test_serializeDeserialize();
    test_markDeleted();
    std::cout << "\n\n";
}


