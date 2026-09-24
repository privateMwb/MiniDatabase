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

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Verifies that setField followed by getField returns the same value back.
TEST(Record, FieldSetGetRoundtrip) {
    Record r(1);

    ASSERT_EQ(r.setField("age", Json(30)), Status::OK);
    ASSERT_EQ(r.setField("name", Json("Ada")), Status::OK);
    ASSERT_EQ(r.setField("active", Json(true)), Status::OK);

    EXPECT_EQ(r.getField("age").asNumber(), 30);
    EXPECT_EQ(r.getField("name").asString(), "Ada");
    EXPECT_EQ(r.getField("active").asBool(), true);
}

// Verifies that getFieldRef returns the same value as getField for a
// present field (the no-copy accessor must agree with the copying one).
TEST(Record, FieldRefMatchesValue) {
    Record r(1);
    ASSERT_EQ(r.setField("score", Json(97.5)), Status::OK);

    const Json& ref = r.getFieldRef("score");
    Json val = r.getField("score");

    ASSERT_TRUE(ref.isNumber());
    ASSERT_TRUE(val.isNumber());
    EXPECT_EQ(ref.asNumber(), val.asNumber());
}

// Verifies that getFieldRef on a missing key returns a null Json rather
// than throwing or aborting.
TEST(Record, FieldRefMissingReturnsNull) {
    Record r(1);
    const Json& ref = r.getFieldRef("does_not_exist");

    EXPECT_TRUE(ref.isNull());
}

// Verifies hasField reports presence correctly before and after setField.
TEST(Record, HasFieldBasic) {
    Record r(1);
    ASSERT_FALSE(r.hasField("x"));

    ASSERT_EQ(r.setField("x", Json(1)), Status::OK);
    EXPECT_TRUE(r.hasField("x"));
}

// Verifies that removeField deletes an existing field.
TEST(Record, RemoveFieldExisting) {
    Record r(1);
    ASSERT_EQ(r.setField("x", Json(1)), Status::OK);

    ASSERT_EQ(r.removeField("x"), Status::OK);
    EXPECT_FALSE(r.hasField("x"));
}

// Verifies that removeField on a missing key reports NOT_FOUND rather than
// silently succeeding.
TEST(Record, RemoveFieldMissing) {
    Record r(1);
    EXPECT_EQ(r.removeField("nope"), Status::NOT_FOUND);
}

// Verifies that validate() accepts an INT column whose value is exactly 0.
// Regression coverage for the bug where validate() used
// `static_cast<int64_t>(val.asNumber())` as a truthiness check, which
// rejected the valid value 0.
TEST(Record, ValidateIntAcceptsZero) {
    Record r(1);
    ASSERT_EQ(r.setField("count", Json(0)), Status::OK);

    Vector<ColumnDef> schema{ColumnDef{"count", ColumnType::INT, false}};
    EXPECT_EQ(r.validate(schema), Status::OK);
}

// Verifies that validate() rejects an INT column whose value has a
// fractional part.
TEST(Record, ValidateIntRejectsFraction) {
    Record r(1);
    ASSERT_EQ(r.setField("count", Json(1.5)), Status::OK);

    Vector<ColumnDef> schema{ColumnDef{"count", ColumnType::INT, false}};
    EXPECT_NE(r.validate(schema), Status::OK);
}

// Verifies that a missing nullable column passes validation.
TEST(Record, ValidateMissingNullableOk) {
    Record r(1); // no fields set

    Vector<ColumnDef> schema{ColumnDef{"nickname", ColumnType::STRING, true}};
    EXPECT_EQ(r.validate(schema), Status::OK);
}

// Verifies that a missing non-nullable column fails validation.
TEST(Record, ValidateMissingRequiredFails) {
    Record r(1); // no fields set

    Vector<ColumnDef> schema{ColumnDef{"email", ColumnType::STRING, false}};
    EXPECT_EQ(r.validate(schema), Status::INVALID_SCHEMA);
}

// Verifies that a field with the wrong JSON type for its column fails
// validation (STRING column holding a number).
TEST(Record, ValidateWrongTypeFails) {
    Record r(1);
    ASSERT_EQ(r.setField("name", Json(123)), Status::OK);

    Vector<ColumnDef> schema{ColumnDef{"name", ColumnType::STRING, false}};
    EXPECT_NE(r.validate(schema), Status::OK);
}

// Verifies that toJson()/fromJson() preserves id, deleted flag, and data.
TEST(Record, JsonRoundTrip) {
    Record original(7);
    ASSERT_EQ(original.setField("x", Json(42)), Status::OK);
    original.markDeleted();

    Json envelope = original.toJson();

    Record restored;
    ASSERT_EQ(restored.fromJson(envelope), Status::OK);

    EXPECT_EQ(restored.getID(), 7);
    EXPECT_TRUE(restored.isDeleted());
    EXPECT_EQ(restored.getField("x").asNumber(), 42);
}

// Verifies that serialize()/deserialize() round trip matches the
// toJson()/fromJson() round trip (they should be equivalent, since
// serialize() is meant to be a thin dump() wrapper around toJson()).
TEST(Record, SerializeRoundTrip) {
    Record original(3);
    ASSERT_EQ(original.setField("label", Json("hello")), Status::OK);

    std::string raw = original.serialize();

    Record restored;
    ASSERT_EQ(restored.deserialize(raw), Status::OK);

    EXPECT_EQ(restored.getID(), 3);
    EXPECT_EQ(restored.getField("label").asString(), "hello");
}

// Verifies that deserialize() on malformed JSON reports PARSE_ERROR rather
// than crashing or silently producing a garbage Record.
TEST(Record, DeserializeMalformedReturnsParseError) {
    Record r;
    EXPECT_EQ(r.deserialize("{not valid json"), Status::PARSE_ERROR);
}

// Verifies markDeleted()/isDeleted() basic behavior.
TEST(Record, DeletedFlagBasic) {
    Record r(1);
    ASSERT_FALSE(r.isDeleted());

    r.markDeleted();
    EXPECT_TRUE(r.isDeleted());
}
