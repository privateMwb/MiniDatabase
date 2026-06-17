#include "TestHelpers.h"
#include <cassert>
#include <iostream>

void test_constructDefault() {
    Record r;
    assert(r.getID() == DBConstants::INVALID_RECORD_ID);
    assert(!r.isDeleted());
    std::cout << "test_constructDefault passed\n";
}

void test_setAndGetField() {
    Record r(1);
    Status s = r.setField("name", Json("Alice"));

    assert(s == Status::OK);
    assert(r.hasField("name"));
    assert(r.getField("name").asString() == "Alice");
    std::cout << "test_setAndGetField passed\n";
}

void test_validateSchema() {
    VectorPro<ColumnDef> schema = makeEmployeeSchema();

    Record valid = makeEmployeeRecord(1, "Bob", 60000);
    assert(valid.validate(schema) == Status::OK);

    Record missingField(2);
    missingField.setField("name", Json("Charlie"));
    // missing required "salary"
    assert(missingField.validate(schema) == Status::INVALID_SCHEMA);

    Record wrongType(3);
    wrongType.setField("name", Json("Dana"));
    wrongType.setField("salary", Json("not a number"));
    assert(wrongType.validate(schema) == Status::INVALID_TYPE);

    std::cout << "test_validateSchema passed\n";
}

void test_removeField() {
    Record r(1);
    r.setField("name", Json("Alice"));

    Status s = r.removeField("name");
    assert(s == Status::OK);
    assert(!r.hasField("name"));

    Status missing = r.removeField("name");
    assert(missing == Status::NOT_FOUND);
    std::cout << "test_removeField passed\n";
}


void test_serializeDeserialize() {
    Record original = makeEmployeeRecord(5, "Eve", 75000);
    std::string raw = original.serialize();

    Record restored;
    Status s = restored.deserialize(raw);

    assert(s == Status::OK);
    assert(restored.getID() == 5);
    assert(restored.getField("name").asString() == "Eve");
    assert(restored.getField("salary").asNumber() == 75000);

    std::cout << "test_serializeDeserialize passed\n";
}

void test_markDeleted() {
    Record r(1);
    assert(!r.isDeleted());

    r.markDeleted();
    assert(r.isDeleted());

    std::cout << "test_markDeleted passed\n";
}

void run_record_tests() {
    std::cout << "--- Record Tests ---\n";
    test_constructDefault();
    test_setAndGetField();
    test_removeField();
    test_validateSchema();
    test_serializeDeserialize();
    test_markDeleted();
    std::cout << "--- Record Tests Complete ---\n\n";
}


