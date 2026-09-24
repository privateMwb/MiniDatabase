// Record Test Suite
// Verifies field access, schema validation, and serialization for a single
// Record in isolation (no Page/Table/Database, no file I/O).
//
// Covers:
// - setField / getField round trip
// - getFieldRef parity with getField, and its behavior on a missing key
// - hasField / removeField
// - validate() against a schema: correct types, wrong types, nullable vs
//   required missing fields, and the INT-accepts-zero fix
// - toJson / fromJson round trip
// - serialize / deserialize round trip
// - deserialize on malformed input
// - the deleted flag

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Verifies that setField followed by getField returns the same value back.
static void field_set_get_roundtrip() {
    Record r(1);

    CHK(r.setField("age", Json(30)) == Status::OK);
    CHK(r.setField("name", Json("Ada")) == Status::OK);
    CHK(r.setField("active", Json(true)) == Status::OK);

    CHK(r.getField("age").asNumber() == 30);
    CHK(r.getField("name").asString() == "Ada");
    CHK(r.getField("active").asBool() == true);
}

// Verifies that getFieldRef returns the same value as getField for a
// present field (the no-copy accessor must agree with the copying one).
static void field_ref_matches_value() {
    Record r(1);
    CHK(r.setField("score", Json(97.5)) == Status::OK);

    const Json& ref = r.getFieldRef("score");
    Json val = r.getField("score");

    CHK(ref.isNumber());
    CHK(val.isNumber());
    CHK(ref.asNumber() == val.asNumber());
}

// Verifies that getFieldRef on a missing key returns a null Json rather
// than throwing or aborting.
static void field_ref_missing_returns_null() {
    Record r(1);
    const Json& ref = r.getFieldRef("does_not_exist");

    CHK(ref.isNull());
}

// Verifies hasField reports presence correctly before and after setField.
static void has_field_basic() {
    Record r(1);
    CHK(r.hasField("x") == false);

    CHK(r.setField("x", Json(1)) == Status::OK);
    CHK(r.hasField("x") == true);
}

// Verifies that removeField deletes an existing field.
static void remove_field_existing() {
    Record r(1);
    CHK(r.setField("x", Json(1)) == Status::OK);

    CHK(r.removeField("x") == Status::OK);
    CHK(r.hasField("x") == false);
}

// Verifies that removeField on a missing key reports NOT_FOUND rather than
// silently succeeding.
static void remove_field_missing() {
    Record r(1);
    CHK(r.removeField("nope") == Status::NOT_FOUND);
}

// Verifies that validate() accepts an INT column whose value is exactly 0.
// Regression coverage for the bug where validate() used
// `static_cast<int64_t>(val.asNumber())` as a truthiness check, which
// rejected the valid value 0.
static void validate_int_accepts_zero() {
    Record r(1);
    CHK(r.setField("count", Json(0)) == Status::OK);

    Vector<ColumnDef> schema{ColumnDef{"count", ColumnType::INT, false}};
    CHK(r.validate(schema) == Status::OK);
}

// Verifies that validate() rejects an INT column whose value has a
// fractional part.
static void validate_int_rejects_fraction() {
    Record r(1);
    CHK(r.setField("count", Json(1.5)) == Status::OK);

    Vector<ColumnDef> schema{ColumnDef{"count", ColumnType::INT, false}};
    CHK(r.validate(schema) != Status::OK);
}

// Verifies that a missing nullable column passes validation.
static void validate_missing_nullable_ok() {
    Record r(1); // no fields set

    Vector<ColumnDef> schema{ColumnDef{"nickname", ColumnType::STRING, true}};
    CHK(r.validate(schema) == Status::OK);
}

// Verifies that a missing non-nullable column fails validation.
static void validate_missing_required_fails() {
    Record r(1); // no fields set

    Vector<ColumnDef> schema{ColumnDef{"email", ColumnType::STRING, false}};
    CHK(r.validate(schema) == Status::INVALID_SCHEMA);
}

// Verifies that a field with the wrong JSON type for its column fails
// validation (STRING column holding a number).
static void validate_wrong_type_fails() {
    Record r(1);
    CHK(r.setField("name", Json(123)) == Status::OK);

    Vector<ColumnDef> schema{ColumnDef{"name", ColumnType::STRING, false}};
    CHK(r.validate(schema) != Status::OK);
}

// Verifies that toJson()/fromJson() preserves id, deleted flag, and data.
static void json_round_trip() {
    Record original(7);
    CHK(original.setField("x", Json(42)) == Status::OK);
    original.markDeleted();

    Json envelope = original.toJson();

    Record restored;
    CHK(restored.fromJson(envelope) == Status::OK);

    CHK(restored.getID() == 7);
    CHK(restored.isDeleted() == true);
    CHK(restored.getField("x").asNumber() == 42);
}

// Verifies that serialize()/deserialize() round trip matches the
// toJson()/fromJson() round trip (they should be equivalent, since
// serialize() is meant to be a thin dump() wrapper around toJson()).
static void serialize_round_trip() {
    Record original(3);
    CHK(original.setField("label", Json("hello")) == Status::OK);

    std::string raw = original.serialize();

    Record restored;
    CHK(restored.deserialize(raw) == Status::OK);

    CHK(restored.getID() == 3);
    CHK(restored.getField("label").asString() == "hello");
}

// Verifies that deserialize() on malformed JSON reports PARSE_ERROR rather
// than crashing or silently producing a garbage Record.
static void deserialize_malformed_returns_parse_error() {
    Record r;
    CHK(r.deserialize("{not valid json") == Status::PARSE_ERROR);
}

// Verifies markDeleted()/isDeleted() basic behavior.
static void deleted_flag_basic() {
    Record r(1);
    CHK(r.isDeleted() == false);

    r.markDeleted();
    CHK(r.isDeleted() == true);
}

// Executes all Record test cases.
static void run_tests() {
    RUN(field_set_get_roundtrip);
    RUN(field_ref_matches_value);
    RUN(field_ref_missing_returns_null);
    RUN(has_field_basic);
    RUN(remove_field_existing);
    RUN(remove_field_missing);
    RUN(validate_int_accepts_zero);
    RUN(validate_int_rejects_fraction);
    RUN(validate_missing_nullable_ok);
    RUN(validate_missing_required_fails);
    RUN(validate_wrong_type_fails);
    RUN(json_round_trip);
    RUN(serialize_round_trip);
    RUN(deserialize_malformed_returns_parse_error);
    RUN(deleted_flag_basic);
}

REGISTER_TEST_SUITE();
