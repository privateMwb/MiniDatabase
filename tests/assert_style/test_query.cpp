// QueryEngine Test Suite
//
// Comprehensive test suite for the QueryEngine implementation.
//
// Covers:
// - Selecting all records
// - Selecting a single record by ID
// - Filtering with single and multiple predicates
// - Sorting results ascending and descending
// - Limiting result counts
// - Aggregate functions: count, sum, avg, max, min
//
// Each test validates behavior using assertions and prints a
// success message when completed.

#include "TestHelpers.h"
#include "../../include/Engine/QueryEngine.h"
#include "../../libs/CustomAllocators/ArenaAllocator/ArenaAllocator.h"

#include <cassert>
#include <iostream>

// Select All
// Verifies that selectAll returns every live record in the table.
static void test_selectAll() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    QueryResult result = qe.selectAll(table);

    assert(result.status == Status::OK);
    assert(result.records.size() == 2);
    assert(result.totalMatched == 2);

    std::cout << "[PASS] selectAll\n";
}

// Select By Id
// Verifies that selectById returns the correct single record, or NOT_FOUND when missing.
static void test_selectById() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(7, "Carol", 70000));

    QueryResult found = qe.selectByID(table, 7);
    assert(found.status == Status::OK);
    assert(found.records.size() == 1);
    assert(found.records[0].getField("name").asString() == "Carol");

    QueryResult missing = qe.selectByID(table, 999);
    assert(missing.status == Status::NOT_FOUND);
    assert(missing.records.size() == 0);

    std::cout << "[PASS] selectById\n";
}

// Select With Single Predicate
// Verifies that select correctly filters records using one condition.
static void test_selectSinglePredicate() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 80000));
    (void)table.insertRecord(makeEmployeeRecord(3, "Carol", 90000));

    VectorPro<FilterPredicate*> predicates;
    FilterPredicate gtFilter{"salary", Op::GT, Json(70000.0)};
    predicates.push_back(&gtFilter);

    QueryResult result = qe.select(table, predicates);

    assert(result.status == Status::OK);
    assert(result.records.size() == 2);  // Bob and Carol

    std::cout << "[PASS] selectSinglePredicate\n";
}

// Select With Multiple Predicates
// Verifies that select AND-combines multiple conditions correctly.
static void test_selectMultiplePredicates() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 80000));
    (void)table.insertRecord(makeEmployeeRecord(3, "Carol", 90000));

    VectorPro<FilterPredicate*> predicates;
    FilterPredicate gtFilter{"salary", Op::GT, Json(50000.0)};
    FilterPredicate ltFilter{"salary", Op::LT, Json(90000.0)};
    predicates.push_back(&gtFilter);
    predicates.push_back(&ltFilter);

    QueryResult result = qe.select(table, predicates);

    assert(result.status == Status::OK);
    assert(result.records.size() == 1);  // only Bob
    assert(result.records[0].getField("name").asString() == "Bob");

    std::cout << "[PASS] selectMultiplePredicates\n";
}

// Select With Sort Ascending
// Verifies that results can be sorted in ascending order by a field.
static void test_selectSortAscending() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Carol", 90000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(3, "Bob", 80000));

    VectorPro<FilterPredicate*> predicates;
    SortCondition sort{"salary", SortOrder::ASC};

    QueryResult result = qe.select(table, predicates, &sort);

    assert(result.status == Status::OK);
    assert(result.records.size() == 3);
    assert(result.records[0].getField("name").asString() == "Alice");
    assert(result.records[1].getField("name").asString() == "Bob");
    assert(result.records[2].getField("name").asString() == "Carol");

    std::cout << "[PASS] selectSortAscending\n";
}

// Select With Sort Descending
// Verifies that results can be sorted in descending order by a field.
static void test_selectSortDescending() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Carol", 90000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(3, "Bob", 80000));

    VectorPro<FilterPredicate*> predicates;
    SortCondition sort{"salary", SortOrder::DESC};

    QueryResult result = qe.select(table, predicates, &sort);

    assert(result.status == Status::OK);
    assert(result.records[0].getField("name").asString() == "Carol");
    assert(result.records[1].getField("name").asString() == "Bob");
    assert(result.records[2].getField("name").asString() == "Alice");

    std::cout << "[PASS] selectSortDescending\n";
}

// Select With Limit
// Verifies that the limit parameter caps the number of returned records.
static void test_selectWithLimit() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    for (std::size_t i = 0; i < 10; i++) {
        (void)table.insertRecord(makeEmployeeRecord(i, "Name", 1000));
    }

    VectorPro<FilterPredicate*> predicates;
    QueryResult result = qe.select(table, predicates, nullptr, 3);

    assert(result.status == Status::OK);
    assert(result.records.size() == 3);

    std::cout << "[PASS] selectWithLimit\n";
}

// Count Aggregate
// Verifies that count returns the correct number of matching records.
static void test_count() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 80000));

    VectorPro<FilterPredicate*> predicates;
    std::size_t total = qe.count(table, predicates);

    assert(total == 2);

    std::cout << "[PASS] count\n";
}

// Sum Aggregate
// Verifies that sum correctly totals a numeric field across matching records.
static void test_sum() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 80000));

    VectorPro<FilterPredicate*> predicates;
    double total = qe.sum(table, "salary", predicates);

    assert(total == 130000);

    std::cout << "[PASS] sum\n";
}

// Avg Aggregate
// Verifies that avg correctly averages a numeric field across matching records.
static void test_avg() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 100000));

    VectorPro<FilterPredicate*> predicates;
    double average = qe.avg(table, "salary", predicates);

    assert(average == 75000);

    std::cout << "[PASS] avg\n";
}

// Max Aggregate
// Verifies that max returns the highest value of a numeric field.
static void test_max() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 90000));
    (void)table.insertRecord(makeEmployeeRecord(3, "Carol", 70000));

    VectorPro<FilterPredicate*> predicates;
    double highest = qe.max(table, "salary", predicates);

    assert(highest == 90000);

    std::cout << "[PASS] max\n";
}

// Min Aggregate
// Verifies that min returns the lowest value of a numeric field.
static void test_min() {
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    Table table = makeEmployeeTable();
    (void)table.insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)table.insertRecord(makeEmployeeRecord(2, "Bob", 90000));
    (void)table.insertRecord(makeEmployeeRecord(3, "Carol", 70000));

    VectorPro<FilterPredicate*> predicates;
    double lowest = qe.min(table, "salary", predicates);

    assert(lowest == 50000);

    std::cout << "[PASS] min\n";
}

// Run Query Tests
// Executes all query engine unit tests and reports their results.
void run_query_tests() {
    std::cout << "Query Tests\n";
    test_selectAll();
    test_selectById();
    test_selectSinglePredicate();
    test_selectMultiplePredicates();
    test_selectSortAscending();
    test_selectSortDescending();
    test_selectWithLimit();
    test_count();
    test_sum();
    test_avg();
    test_max();
    test_min();
    std::cout << "\n\n";
}
