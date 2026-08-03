// Table Copy-Assignment Signature Regression Test
// Pins the fix for a signature typo: `Table& operator=(const Table&&)`
// does NOT suppress the compiler-generated real copy-assignment operator
// `Table& operator=(const Table&)` -- it just adds an unrelated,
// essentially unreachable overload for const rvalues. Table silently
// remained copy-assignable via the implicit operator. Fixed to
// `Table& operator=(const Table&) = delete;`.
//
// Covers:
// - Table is not copy-assignable or copy-constructible, and is still
//   move-assignable, checked at compile time (a regression here fails to
//   COMPILE, not just fails a runtime check)
// - the move-assignment path -- the only assignment path Table has left --
//   still transfers state correctly, so the type trait above isn't true by
//   accident of a broken move too

#include <support/framework.h>

#include <type_traits>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Compile-time pin: if the `const Table&&` typo regresses, this fails to
// COMPILE, not just fails a runtime check.
static void table_copy_assign_deleted() {
    static_assert(!std::is_copy_assignable_v<Table>,
                  "Table must not be copy-assignable "
                  "(regression of the `const Table&&` signature typo)");
    static_assert(!std::is_copy_constructible_v<Table>, "Table must not be copy-constructible");
    static_assert(std::is_move_assignable_v<Table>, "Table must still be move-assignable");

    CHK(true); // the static_asserts above are the real assertions
}

// Verifies move assignment -- the only assignment path Table has left --
// actually transfers state correctly.
static void table_move_assign_transfers_state() {
    Table source("orders", 1, Vector<ColumnDef>{});
    CHK(source.insertRecord(Record(1)) == Status::OK);

    Table target("empty", 2, Vector<ColumnDef>{});
    target = std::move(source);

    CHK(target.getName() == "orders");
    CHK(target.recordCount() == 1);

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    CHK(source.getID() == DBConstants::INVALID_TABLE_ID);
}

static void run_tests() {
    RUN(table_copy_assign_deleted);
    RUN(table_move_assign_transfers_state);
}

REGISTER_TEST_SUITE();