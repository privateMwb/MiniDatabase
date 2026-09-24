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

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static void validate_int_accepts_zero() {
    Vector<ColumnDef> schema{ColumnDef{"balance", ColumnType::INT, false}};
    Record r(1);
    CHK(r.setField("balance", Json(0)) == Status::OK);
    CHK(r.validate(schema) == Status::OK);
}

static void validate_int_accepts_nonzero() {
    Vector<ColumnDef> schema{ColumnDef{"balance", ColumnType::INT, false}};

    Record zero(1);
    CHK(zero.setField("balance", Json(0)) == Status::OK);
    CHK(zero.validate(schema) == Status::OK);

    Record nonzero(2);
    CHK(nonzero.setField("balance", Json(42)) == Status::OK);
    CHK(nonzero.validate(schema) == Status::OK);
}

static void validate_int_rejects_fraction() {
    Vector<ColumnDef> schema{ColumnDef{"balance", ColumnType::INT, false}};
    Record r(1);
    CHK(r.setField("balance", Json(1.5)) == Status::OK);
    CHK(r.validate(schema) == Status::INVALID_TYPE);
}

static void run_tests() {
    RUN(validate_int_accepts_zero);
    RUN(validate_int_accepts_nonzero);
    RUN(validate_int_rejects_fraction);
}

REGISTER_TEST_SUITE();