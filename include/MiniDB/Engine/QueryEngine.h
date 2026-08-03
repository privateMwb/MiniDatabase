/**
 * @file            QueryEngine.h
 *
 * @date            2026-2-8
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 MWB
 *                  All rights reserved.
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

// clang-format off
#include <string>    // std::string
#include <functional> // (predicate/comparator support)
#include <span>      // std::span (predicate lists)
// clang-format on

#include <MiniDB/Common/Type.h>
#include <MiniDB/Core/Record.h>
#include <MiniDB/Core/Table.h>

// clang-format off
#include <VectorPro/Vector.h>       // VectorPro::Vector (result records)
#include <JsonPro/Json.h>           // JsonPro::Json (predicate/field values)
#include <ArenaPro/Arena.h>         // ArenaPro::Arena (per-query scratch memory)
#include <ArenaPro/ArenaScope.h>    // ArenaPro::ArenaScope (RAII arena reset)
// clang-format on

// Predicate-filtered, optionally sorted table scans, plus simple aggregates
// (count/sum/avg/max/min). Every scan is a full page-by-page, record-by-
// record walk of the table -- there's no index-assisted lookup beyond
// Table's own RecordID index -- with an ArenaScope bounding any scratch
// allocation made during the scan to the query's own lifetime.

namespace MiniDB::Engine {

using namespace MiniDB::Common;
using namespace MiniDB::Core;
using namespace VectorPro;
using namespace JsonPro;
using namespace ArenaPro;

/// @brief A single filter condition: `field <op> value`.
struct FilterPredicate {
    FieldName field;
    Op op;
    Json value;
};

/// @brief A single sort condition: order results by `field` in `order`.
struct SortCondition {
    FieldName field;
    SortOrder order;
};

/// @brief The outcome of a query: matched records plus a status.
struct QueryResult {
    Vector<Record> records;       ///< Matched records, in scan (or post-sort) order.
    std::size_t totalMatched = 0; ///< Number of records that matched, before any `limit` cutoff.
    Status status = Status::OK;   ///< `Status::OK` on success, or a `Table`/`Record` error status.
};

/**
 * @brief Executes predicate-filtered scans, sorts, and simple aggregates
 * over a `Table`.
 * @details Every query walks `table.getPages()` page by page and record
 * by record — there's no index beyond `Table`'s own `RecordID` index —
 * skipping soft-deleted records and evaluating each `FilterPredicate` in
 * turn via `evaluateAll()`. Each call scopes an `ArenaScope` over its
 * scan, so any scratch memory allocated from `arena` during the query is
 * released when the call returns, independent of how the caller manages
 * the arena elsewhere.
 */
class QueryEngine {
  private:
    Arena<>& arena; ///< Scratch arena scoped per-call via `ArenaScope`.

  public:
    /// @brief Constructs a query engine that scopes its scratch
    /// allocations from `arena`.
    /// @param arena Arena to allocate scratch memory from during queries.
    explicit QueryEngine(Arena<>& arena);

    /**
     * @brief Scans `table`, returning records matching every predicate
     * in `predicates`, optionally sorted and/or capped.
     * @param table Table to scan.
     * @param predicates Filter conditions; a record must satisfy all of
     * them (AND semantics) to match. Empty span matches every
     * non-deleted record.
     * @param sort Optional sort condition applied to the matched records
     * after the scan. `nullptr` (the default) leaves results in scan
     * order.
     * @param limit Maximum number of records to collect; `0` (the
     * default) means unlimited. When reached, the scan stops early
     * (before the rest of the table is walked) and the sort, if any, is
     * still applied to the collected subset.
     * @return A `QueryResult` with `status == Status::OK`.
     */
    [[nodiscard]] QueryResult select(Table& table, std::span<const FilterPredicate> predicates,
                                     const SortCondition* sort = nullptr, std::size_t limit = 0);

    /// @brief Returns every non-deleted record in `table`, unsorted, unfiltered.
    /// @param table Table to scan.
    [[nodiscard]] QueryResult selectAll(Table& table);

    /**
     * @brief Looks up a single record by id.
     * @param table Table to search.
     * @param id Record id to look up.
     * @return A `QueryResult` containing the record and
     * `status == Status::OK` on success, or an empty result with
     * whatever `Status` `Table::getRecord()` returned on failure.
     */
    [[nodiscard]] QueryResult selectByID(Table& table, RecordID id);

    /**
     * @brief Counts non-deleted records in `table` matching every
     * predicate in `predicates`.
     * @param table Table to scan.
     * @param predicates Filter conditions (AND semantics). Empty span
     * counts every non-deleted record.
     * @return The number of matching records.
     */
    [[nodiscard]] std::size_t count(Table& table, std::span<const FilterPredicate> predicates);

    /**
     * @brief Sums `field` across matching records.
     * @param table Table to scan.
     * @param field Field name to sum. Records missing this field, or
     * where it isn't a number, are skipped.
     * @param predicates Filter conditions (AND semantics).
     * @return The sum, or `0.0` if no matching record has a numeric
     * value for `field`.
     */
    [[nodiscard]] double sum(Table& table, const FieldName& field,
                             std::span<const FilterPredicate> predicates);

    /**
     * @brief Averages `field` across matching records.
     * @param table Table to scan.
     * @param field Field name to average. Records missing this field, or
     * where it isn't a number, are skipped.
     * @param predicates Filter conditions (AND semantics).
     * @return The average, or `0.0` if no matching record has a numeric
     * value for `field`.
     */
    [[nodiscard]] double avg(Table& table, const FieldName& field,
                             std::span<const FilterPredicate> predicates);

    /**
     * @brief Finds the maximum value of `field` across matching records.
     * @param table Table to scan.
     * @param field Field name to compare. Records missing this field, or
     * where it isn't a number, are skipped.
     * @param predicates Filter conditions (AND semantics).
     * @return The maximum value found, or `0.0` if no matching record
     * has a numeric value for `field`.
     */
    [[nodiscard]] double max(Table& table, const FieldName& field,
                             std::span<const FilterPredicate> predicates);

    /**
     * @brief Finds the minimum value of `field` across matching records.
     * @param table Table to scan.
     * @param field Field name to compare. Records missing this field, or
     * where it isn't a number, are skipped.
     * @param predicates Filter conditions (AND semantics).
     * @return The minimum value found, or `0.0` if no matching record
     * has a numeric value for `field`.
     */
    [[nodiscard]] double min(Table& table, const FieldName& field,
                             std::span<const FilterPredicate> predicates);

  private:
    /**
     * @brief Applies an optional sort to `result.records` and returns it.
     * @param result Result to finish. Consumed and returned by value.
     * @param sort Sort condition to apply, or `nullptr` to leave `result`
     * unsorted.
     * @return `result`, sorted if `sort` was non-null and it held more
     * than one record.
     * @details Runs once, after the scan loop completes — a single exit
     * path rather than a sort duplicated at every early-return site.
     */
    [[nodiscard]] QueryResult finishSelect(QueryResult result, const SortCondition* sort) const;

    /**
     * @brief Evaluates a single predicate against a record.
     * @param record Record to test.
     * @param predicate Predicate to evaluate.
     * @return `true` if `record` has `predicate.field` and its value
     * satisfies `predicate.op` against `predicate.value`.
     */
    [[nodiscard]] bool evaluatePredicate(const Record& record,
                                         const FilterPredicate& predicate) const;

    /**
     * @brief Evaluates every predicate against a record (AND semantics).
     * @param record Record to test.
     * @param predicates Predicates to evaluate. Passed as a contiguous
     * span rather than a container of pointers, so no per-predicate
     * indirection or forced heap allocation is imposed on callers.
     * @return `true` if `record` satisfies every predicate, or
     * `predicates` is empty.
     */
    [[nodiscard]] bool evaluateAll(const Record& record,
                                   std::span<const FilterPredicate> predicates) const;

    /**
     * @brief Sorts `records` in place according to `sort`.
     * @param records Records to sort.
     * @param sort Field and direction to sort by.
     * @details Uses `std::sort` (O(n log n)) comparing via
     * `compareValues()` on each record's `sort.field`.
     */
    void sortResults(Vector<Record>& records, const SortCondition& sort) const;

    /**
     * @brief Compares two `Json` values under a given operator.
     * @param a Left-hand value.
     * @param b Right-hand value.
     * @param op Comparison operator to apply.
     * @return The result of `a <op> b`. `false` if `a` and `b` aren't
     * both strings, both numbers, or both bools (mismatched/unsupported
     * types never compare true), or if `op` isn't meaningful for bools
     * (only `EQ`/`NEQ` are supported there).
     */
    [[nodiscard]] bool compareValues(const Json& a, const Json& b, Op op) const;
};

} // namespace MiniDB::Engine
