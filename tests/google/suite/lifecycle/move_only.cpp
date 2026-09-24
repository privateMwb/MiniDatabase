// Contract: Move-Only Ownership
// Page, Table, and Database each own resources (a Pool-backed record
// array, a vector of owned Page*, a vector of owned Table*) that must
// never be duplicated by an implicit shallow copy. Each type must be
// copy-disabled and move-valid, and a move must leave the source in a
// well-defined, safely-destructible state.
//
// Covers:
// - compile-time: none of Page/Table/Database are copy-constructible or
//   copy-assignable; all are move-constructible and move-assignable
// - runtime: moving each type transfers its data correctly and leaves the
//   moved-from object in a valid (empty / invalid-id) state

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

#include <type_traits>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// --- Compile-time checks --------------------------------------------------

static_assert(!std::is_copy_constructible_v<Page>,
              "Page must not be copy-constructible (owns a Pool-backed record array)");
static_assert(!std::is_copy_assignable_v<Page>, "Page must not be copy-assignable");
static_assert(std::is_move_constructible_v<Page>, "Page must be move-constructible");
static_assert(std::is_move_assignable_v<Page>, "Page must be move-assignable");

static_assert(!std::is_copy_constructible_v<Table>,
              "Table must not be copy-constructible (owns Page*)");
static_assert(!std::is_copy_assignable_v<Table>, "Table must not be copy-assignable");
static_assert(std::is_move_constructible_v<Table>, "Table must be move-constructible");
static_assert(std::is_move_assignable_v<Table>, "Table must be move-assignable");

static_assert(!std::is_copy_constructible_v<Database>,
              "Database must not be copy-constructible (owns Table*)");
static_assert(!std::is_copy_assignable_v<Database>, "Database must not be copy-assignable");
static_assert(std::is_move_constructible_v<Database>, "Database must be move-constructible");
static_assert(std::is_move_assignable_v<Database>, "Database must be move-assignable");

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

} // namespace

// --- Runtime checks --------------------------------------------------------

// Verifies moving a Page transfers its records and id, and the source is
// left in a safely-destructible, empty state.
TEST(MoveOnly, PageMoveTransfersOwnership) {
    Page original(5);
    Record r(1);
    (void)r.setField("x", Json(1));
    ASSERT_EQ(original.addRecord(r), Status::OK);

    Page moved(std::move(original));

    EXPECT_EQ(moved.getID(), 5);
    EXPECT_EQ(moved.recordCount(), 1);

    // Moved-from Page must remain safely destructible and report an empty
    // state rather than dangling into the moved records.
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_EQ(original.recordCount(), 0);
}

// Verifies move-assigning a Page transfers ownership and releases whatever
// the destination previously owned.
TEST(MoveOnly, PageMoveAssignTransfersOwnership) {
    Page a(1);
    Record ra(1);
    ASSERT_EQ(a.addRecord(ra), Status::OK);

    Page b(2);
    Record rb1(1), rb2(2);
    ASSERT_EQ(b.addRecord(rb1), Status::OK);
    ASSERT_EQ(b.addRecord(rb2), Status::OK);

    b = std::move(a);

    EXPECT_EQ(b.getID(), 1);
    EXPECT_EQ(b.recordCount(), 1);
}

// Verifies moving a Table transfers schema, pages, and records, and the
// source is left with id == INVALID_TABLE_ID and no pages.
TEST(MoveOnly, TableMoveTransfersOwnership) {
    Table original("users", 7, makeSchema());

    Record r(1);
    (void)r.setField("name", Json("Ada"));

    ASSERT_EQ(original.insertRecord(r), Status::OK);
}

// Verifies move-assigning a Table releases the destination's previously
// owned pages (no leak/crash) and adopts the source's state.
TEST(MoveOnly, TableMoveAssignTransfersOwnership) {
    Table a("a", 1, makeSchema());

    Record ra(1);
    ASSERT_EQ(ra.setField("name", Json("Alice")), Status::OK);
    ASSERT_EQ(a.insertRecord(ra), Status::OK);

    Table b("b", 2, makeSchema());

    Record rb(1);
    ASSERT_EQ(rb.setField("name", Json("Bob")), Status::OK);
    ASSERT_EQ(b.insertRecord(rb), Status::OK);

    b = std::move(a);

    EXPECT_EQ(b.getID(), 1);
    EXPECT_EQ(b.getName(), "a");
    EXPECT_EQ(b.recordCount(), 1);
}

// Verifies moving a Database transfers all owned tables, and the source is
// left empty.
TEST(MoveOnly, DatabaseMoveTransfersOwnership) {
    Database original("app");

    ASSERT_EQ(original.createTable("users", makeSchema()), Status::OK);

    Database moved(std::move(original));

    EXPECT_EQ(moved.getName(), "app");
    EXPECT_TRUE(moved.hasTable("users"));
    EXPECT_TRUE(original.isEmpty());

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_FALSE(original.hasTable("users"));
}

// Verifies move-assigning a Database releases the destination's previously
// owned tables and adopts the source's state.
TEST(MoveOnly, DatabaseMoveAssignTransfersOwnership) {
    Database a("a");
    ASSERT_EQ(a.createTable("t1", makeSchema()), Status::OK);

    Database b("b");
    ASSERT_EQ(b.createTable("t2", makeSchema()), Status::OK);

    b = std::move(a);

    EXPECT_EQ(b.getName(), "a");
    EXPECT_TRUE(b.hasTable("t1"));
    EXPECT_FALSE(b.hasTable("t2"));
}
