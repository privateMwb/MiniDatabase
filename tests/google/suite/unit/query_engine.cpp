// QueryEngine Test Suite
// Verifies filtering, sorting, limiting, and aggregation over a Table.
//
// Covers:
// - selectAll / selectByID
// - select() with single and multiple (AND-combined) predicates
// - select() respecting `limit`
// - select() sort ordering, ASC and DESC, over more than two records --
//   regression coverage for the sortResults off-by-one bubble-sort bug
//   (the original bug read one element past the end on every call with
//   >= 2 records, so this also implicitly covers "does not crash")
// - count / sum / avg / max / min, including that aggregates skip records
//   missing the target field
//
// ASSUMPTION: Arena<>'s constructor takes a byte-size capacity, matching
// the DBConstants::ARENA_SIZE constant ("1 MB per query Arena"). Not
// verified against ArenaPro's header -- if construction differs, this file
// needs `Arena<> arena(...)` adjusted accordingly.

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"age", ColumnType::INT, true},
    };
}

// Builds a table with 5 records: ids 1..5, names "p1".."p5", ages 10,20,
// (missing),40,50 -- id 3 deliberately omits "age" to exercise aggregate
// functions skipping records without the target field.
Table makeTable() {
    Table t("people", 1, makeSchema());
    for (RecordID id = 1; id <= 5; ++id) {
        Record r(id);
        (void)r.setField("name", Json("p" + std::to_string(id)));
        if (id != 3) {
            (void)r.setField("age", Json(static_cast<int>(id) * 10));
        }
        (void)t.insertRecord(r);
    }
    return t;
}

} // namespace

// Verifies selectAll returns every live record.
TEST(QueryEngine, SelectAllReturnsAllRecords) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    QueryResult res = qe.selectAll(t);

    ASSERT_EQ(res.status, Status::OK);
    EXPECT_EQ(res.records.size(), 5);
    EXPECT_EQ(res.totalMatched, 5);
}

// Verifies selectByID finds an existing record and reports NOT_FOUND for a
// missing one.
TEST(QueryEngine, SelectByIdFoundAndMissing) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    QueryResult found = qe.selectByID(t, 2);
    ASSERT_EQ(found.status, Status::OK);
    ASSERT_EQ(found.records.size(), 1);
    EXPECT_EQ(found.records[0].getField("name").asString(), "p2");

    QueryResult missing = qe.selectByID(t, 999);
    EXPECT_EQ(missing.status, Status::NOT_FOUND);
    EXPECT_EQ(missing.records.size(), 0);
}

// Verifies select() with a single equality predicate filters correctly.
TEST(QueryEngine, SelectSinglePredicateEq) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    FilterPredicate pred{"name", Op::EQ, Json("p2")};
    QueryResult res = qe.select(t, std::span<const FilterPredicate>(&pred, 1));

    ASSERT_EQ(res.status, Status::OK);
    ASSERT_EQ(res.records.size(), 1);
    EXPECT_EQ(res.records[0].getField("name").asString(), "p2");
}

// Verifies select() with a single greater-than predicate.
TEST(QueryEngine, SelectSinglePredicateGt) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    FilterPredicate pred{"age", Op::GT, Json(20)};
    QueryResult res = qe.select(t, std::span<const FilterPredicate>(&pred, 1));

    ASSERT_EQ(res.status, Status::OK);
    // ages > 20: id 4 (40), id 5 (50). id 3 has no age and must be excluded.
    EXPECT_EQ(res.records.size(), 2);
}

// Verifies select() combines multiple predicates with AND semantics.
TEST(QueryEngine, SelectMultiplePredicatesAnd) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    FilterPredicate preds[] = {
        {"age", Op::GTE, Json(20)},
        {"name", Op::NEQ, Json("p4")},
    };
    QueryResult res = qe.select(t, std::span<const FilterPredicate>(preds, 2));

    ASSERT_EQ(res.status, Status::OK);
    // age >= 20: ids 2,4,5. Excluding name == "p4" (id 4) leaves ids 2,5.
    EXPECT_EQ(res.records.size(), 2);
}

// Verifies select() with an empty predicate span behaves like selectAll.
TEST(QueryEngine, SelectEmptyPredicatesMatchesAll) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    QueryResult res = qe.select(t, std::span<const FilterPredicate>{});
    ASSERT_EQ(res.status, Status::OK);
    EXPECT_EQ(res.records.size(), 5);
}

// Verifies select() respects the `limit` parameter.
TEST(QueryEngine, SelectRespectsLimit) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    QueryResult res = qe.select(t, std::span<const FilterPredicate>{}, nullptr, 2);
    ASSERT_EQ(res.status, Status::OK);
    EXPECT_EQ(res.records.size(), 2);
}

// Regression: verifies select() with an ASC sort produces fully correct
// non-decreasing order over 5 records (age field, with id 3's missing age
// treated as 0 by compareValues since it's not a number -- documents
// current sort-with-missing-field behavior rather than asserting an
// opinion on what it "should" be).
TEST(QueryEngine, SelectSortAscendingIsCorrect) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    SortCondition sort{"age", SortOrder::ASC};
    QueryResult res = qe.select(t, std::span<const FilterPredicate>{}, &sort);

    ASSERT_EQ(res.status, Status::OK);
    ASSERT_EQ(res.records.size(), 5);
    for (std::size_t i = 1; i < res.records.size(); ++i) {
        const Json& prev = res.records[i - 1].getFieldRef("age");
        const Json& cur = res.records[i].getFieldRef("age");
        if (prev.isNumber() && cur.isNumber()) {
            EXPECT_LE(prev.asNumber(), cur.asNumber());
        }
    }
}

// Regression: same as above but DESC, over the same 5-record set (the
// original off-by-one bug in sortResults read one element past the end of
// the vector on every call with >= 2 records, regardless of direction).
TEST(QueryEngine, SelectSortDescendingIsCorrect) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    SortCondition sort{"name", SortOrder::DESC};
    QueryResult res = qe.select(t, std::span<const FilterPredicate>{}, &sort);

    ASSERT_EQ(res.status, Status::OK);
    ASSERT_EQ(res.records.size(), 5);
    EXPECT_EQ(res.records.front().getField("name").asString(), "p5");
    EXPECT_EQ(res.records.back().getField("name").asString(), "p1");
}

// Verifies count() with a predicate.
TEST(QueryEngine, CountWithPredicate) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    FilterPredicate pred{"age", Op::GTE, Json(20)};
    std::size_t n = qe.count(t, std::span<const FilterPredicate>(&pred, 1));

    EXPECT_EQ(n, 3); // ids 2, 4, 5
}

// Verifies sum/avg/max/min, and that they skip the record missing the
// target field (id 3 has no "age").
TEST(QueryEngine, AggregatesSkipMissingField) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table t = makeTable();

    std::span<const FilterPredicate> noFilter{};

    // ages present: 10, 20, 40, 50 (id 3 excluded)
    EXPECT_EQ(qe.sum(t, "age", noFilter), 120.0);
    EXPECT_EQ(qe.avg(t, "age", noFilter), 30.0);
    EXPECT_EQ(qe.max(t, "age", noFilter), 50.0);
    EXPECT_EQ(qe.min(t, "age", noFilter), 10.0);
}
