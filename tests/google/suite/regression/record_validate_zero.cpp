// Record::validate Zero-Value Regression Test
// Pins the fix where INT-column validation used a truthiness check
// (`static_cast<int64_t>(val.asNumber())`) that rejected the legitimate
// value 0. Fixed by checking is-a-number + no fractional part + fits in
// int64_t, instead of truthiness.
//
// Covers:
// - an INT column holding exactly 0 validates OK
// - non-zero INT values still validate OK (the fix didn't just special-case
//   zero)
// - an INT column holding a fractional value is still correctly rejected

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

TEST(RecordValidateZero, ValidateIntAcceptsZero) {
    Vector<ColumnDef> schema{ColumnDef{"balance", ColumnType::INT, false}};
    Record r(1);
    ASSERT_EQ(r.setField("balance", Json(0)), Status::OK);
    EXPECT_EQ(r.validate(schema), Status::OK);
}

TEST(RecordValidateZero, ValidateIntAcceptsNonzero) {
    Vector<ColumnDef> schema{ColumnDef{"balance", ColumnType::INT, false}};

    Record zero(1);
    ASSERT_EQ(zero.setField("balance", Json(0)), Status::OK);
    EXPECT_EQ(zero.validate(schema), Status::OK);

    Record nonzero(2);
    ASSERT_EQ(nonzero.setField("balance", Json(42)), Status::OK);
    EXPECT_EQ(nonzero.validate(schema), Status::OK);
}

TEST(RecordValidateZero, ValidateIntRejectsFraction) {
    Vector<ColumnDef> schema{ColumnDef{"balance", ColumnType::INT, false}};
    Record r(1);
    ASSERT_EQ(r.setField("balance", Json(1.5)), Status::OK);
    EXPECT_EQ(r.validate(schema), Status::INVALID_TYPE);
}
