// QueryEngine Sort Off-By-One Regression Test
// Pins the fix for the hand-rolled bubble sort's off-by-one inner bound
// (`j < n - i` instead of `j < n - i - 1`), which read one element past the
// end of the array on every call where it ran. Fixed via std::sort.
//
// Covers:
// - a trivial single-record sort completes without error (sortResults is
//   only invoked when records.size() > 1, per finishSelect's guard, so
//   n=1 never actually reached the buggy loop -- included here as a
//   baseline sanity check, not as the bug's repro case)
// - ascending and descending sort produce a fully correct sequence across
//   several sizes starting at n=2 (the actual minimal case that reached
//   the buggy loop: at n=2, i=0, the old bound let j reach 1, reading
//   records[2] -- one past the end), including a two-page size
// - sort still applies correctly on the early-return-then-sort path
//   (limit reached mid-scan), the control flow finishSelect() replaced

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static Table makeScoredTable(std::size_t n, bool descendingInput) {
    Vector<ColumnDef> schema{ColumnDef{"score", ColumnType::INT, false}};
    Table table("t", 1, schema);

    for (RecordID i = 0; i < n; ++i) {
        Record r(i);
        int score = descendingInput ? static_cast<int>(n - i) : static_cast<int>(i);
        (void)r.setField("score", Json(score));
        (void)table.insertRecord(r);
    }
    return table;
}

// Baseline sanity check: a single-record sort completes without error.
// sortResults is only invoked for records.size() > 1 (see finishSelect),
// so this case never actually exercised the buggy loop -- it just confirms
// the trivial path still works after the fix.
TEST(QueryEngineSortOffByOne, SelectSortSingleRecordOk) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    Table table = makeScoredTable(1, /*descendingInput=*/false);
    SortCondition sort{"score", SortOrder::ASC};

    QueryResult result = engine.select(table, std::span<const FilterPredicate>{}, &sort);

    ASSERT_EQ(result.status, Status::OK);
    EXPECT_EQ(result.records.size(), 1);
}

// Verifies ascending sort produces a fully non-decreasing sequence across
// several sizes, including a two-page size.
TEST(QueryEngineSortOffByOne, SelectSortAscendingCorrect) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    for (std::size_t n : {2u, 3u, 5u, DBConstants::MAX_RECORDS_PAGE + 5u}) {
        Table table = makeScoredTable(n, /*descendingInput=*/true);
        SortCondition sort{"score", SortOrder::ASC};

        QueryResult result = engine.select(table, std::span<const FilterPredicate>{}, &sort);

        ASSERT_EQ(result.status, Status::OK);
        ASSERT_EQ(result.records.size(), n);
        for (std::size_t i = 1; i < result.records.size(); ++i) {
            EXPECT_LE(result.records[i - 1].getField("score").asNumber(),
                      result.records[i].getField("score").asNumber());
        }
    }
}

// Verifies descending sort too -- the old bug corrupted memory regardless
// of sort direction, so both directions need coverage.
TEST(QueryEngineSortOffByOne, SelectSortDescendingCorrect) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    for (std::size_t n : {2u, 3u, 5u, DBConstants::MAX_RECORDS_PAGE + 5u}) {
        Table table = makeScoredTable(n, /*descendingInput=*/false);
        SortCondition sort{"score", SortOrder::DESC};

        QueryResult result = engine.select(table, std::span<const FilterPredicate>{}, &sort);

        ASSERT_EQ(result.status, Status::OK);
        ASSERT_EQ(result.records.size(), n);
        for (std::size_t i = 1; i < result.records.size(); ++i) {
            EXPECT_GE(result.records[i - 1].getField("score").asNumber(),
                      result.records[i].getField("score").asNumber());
        }
    }
}

// Verifies sort still applies correctly on the early-return path (limit
// reached mid-scan) -- this is the "goto early-exit-then-sort" control flow
// the fix's comment specifically calls out as replaced by finishSelect().
TEST(QueryEngineSortOffByOne, SelectSortAppliesWithLimit) {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    Table table = makeScoredTable(20, /*descendingInput=*/false);
    SortCondition sort{"score", SortOrder::DESC};

    QueryResult result =
        engine.select(table, std::span<const FilterPredicate>{}, &sort, /*limit=*/5);

    ASSERT_EQ(result.status, Status::OK);
    ASSERT_EQ(result.records.size(), 5);
    for (std::size_t i = 1; i < result.records.size(); ++i) {
        EXPECT_GE(result.records[i - 1].getField("score").asNumber(),
                  result.records[i].getField("score").asNumber());
    }
}
