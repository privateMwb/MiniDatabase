// Page Test Suite
//
// Comprehensive test suite for the Page implementation.
//
// Covers:
// - Record insertion
// - Page capacity limits
// - Record lookup
// - Record updates
// - Handling updates for missing records
// - Record deletion
// - Page compaction
// - Page serialization and deserialization
//
// Each test validates Page behavior using assertions and
// prints a success message when completed.

#include "TestHelpers.h"
#include <cassert>
#include <iostream>

// Add Record
// Verifies that a record can be successfully inserted into an empty page.
static void test_addRecord() {
    Page page(0);
    Record r = makeEmployeeRecord(1, "Alice", 50000);

    Status s = page.addRecord(r);

    assert(s == Status::OK);
    assert(page.recordCount() == 1);
    std::cout << "[PASS] addRecord\n";
}

// Add Record When Full
// Verifies that insertion fails once the page reaches maximum capacity.
static void test_addRecordWhenFull() {
    Page page(0);

    for (std::size_t i = 0; i < DBConstants::MAX_RECORDS_PAGE; i++) {
        Record r = makeEmployeeRecord(i, "Name", 1000);
        Status s = page.addRecord(r);
        assert(s == Status::OK);
    }

    assert(page.isFull());

    Record overflow = makeEmployeeRecord(999, "Overflow", 1000);
    Status s = page.addRecord(overflow);

    assert(s == Status::OUT_OF_MEMORY);
    std::cout << "[PASS] addRecordWhenFull\n";
}

// Get Record
// Verifies that existing records can be retrieved and missing records return nullptr.
static void test_getRecord() {
    Page page(0);
    Record r = makeEmployeeRecord(7, "Bob", 60000);
    (void)page.addRecord(r);

    Record* found = page.getRecord(7);
    assert(found != nullptr);
    assert(found->getID() == 7);
    assert(found->getField("name").asString() == "Bob");

    Record* missing = page.getRecord(999);
    assert(missing == nullptr);

    std::cout << "[PASS] getRecord\n";
}

// Update Record
// Verifies that an existing record can be updated successfully.
static void test_updateRecord() {
    Page page(0);
    Record r = makeEmployeeRecord(3, "Carol", 70000);
    (void)page.addRecord(r);

    Record updated = makeEmployeeRecord(3, "Carol", 85000);
    Status s = page.updateRecord(updated);

    assert(s == Status::OK);

    Record* found = page.getRecord(3);
    assert(found != nullptr);
    assert(found->getField("salary").asNumber() == 85000);

    std::cout << "[PASS] updateRecord\n";
}

// Update Missing Record
// Verifies that updating a non-existent record returns NOT_FOUND.
static void test_updateRecordNotFound() {
    Page page(0);
    Record updated = makeEmployeeRecord(404, "Ghost", 1);

    Status s = page.updateRecord(updated);
    assert(s == Status::NOT_FOUND);

    std::cout << "[PASS] updateRecordNotFound\n";
}

// Delete Record
// Verifies that deleting a record marks it as removed and hides it from lookups.
static void test_deleteRecord() {
    Page page(0);
    Record r = makeEmployeeRecord(9, "Dave", 90000);
    (void)page.addRecord(r);

    Status s = page.deleteRecord(9);
    assert(s == Status::OK);
    assert(page.isDirty());

    Record* found = page.getRecord(9);
    assert(found == nullptr);  // tombstoned records are invisible to getRecord

    std::cout << "[PASS] deleteRecord\n";
}

// Compact Page
// Verifies that deleted records are removed and storage is compacted correctly.
static void test_compact() {
    Page page(0);

    for (std::size_t i = 0; i < 5; i++) {
        Record r = makeEmployeeRecord(i, "Name", 1000);
        (void)page.addRecord(r);
    }

    (void)page.deleteRecord(1);
    (void)page.deleteRecord(3);

    assert(page.recordCount() == 5);  // tombstones still counted before compact

    Status s = page.compact();
    assert(s == Status::OK);
    assert(page.recordCount() == 3);  // only live records remain
    assert(!page.isDirty());

    std::cout << "[PASS] compact\n";
}

// Serialize and Deserialize Page
// Verifies that page contents can be serialized and restored without data loss.
static void test_serializeDeserializePage() {
    Page original(42);
    (void)original.addRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)original.addRecord(makeEmployeeRecord(2, "Bob", 60000));

    std::string raw = original.serialize();

    Page restored(0);
    Status s = restored.deserialize(raw);

    assert(s == Status::OK);
    assert(restored.getID() == 42);
    assert(restored.recordCount() == 2);

    Record* r1 = restored.getRecord(1);
    assert(r1 != nullptr);
    assert(r1->getField("name").asString() == "Alice");

    std::cout << "[PASS] serializeDeserializePage\n";
}

// Run All Page Tests
// Executes every Page test case in sequence.
void run_page_tests() {
    std::cout << "Page Tests\n";
    test_addRecord();
    test_addRecordWhenFull();
    test_getRecord();
    test_updateRecord();
    test_updateRecordNotFound();
    test_deleteRecord();
    test_compact();
    test_serializeDeserializePage();
    std::cout << "\n\n";
}



