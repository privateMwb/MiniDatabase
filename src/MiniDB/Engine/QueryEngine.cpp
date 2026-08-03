/**
 * @file QueryEngine.cpp
 * @brief MiniDB::Engine::QueryEngine implementation.
 *
 * Contains the implementation of predicate-filtered scans, sorting, and
 * aggregate operations over a Table.
 */

// ============================================================
// Implementation for MiniDB::Engine::QueryEngine.
// ============================================================
//
//  Sections:
//   1. Constructor
//   2. Core Query Operation
//   3. Aggregate
//   4. Helpers
//
// ============================================================

#include <MiniDB/Engine/QueryEngine.h>

#include <algorithm>
#include <stdexcept>

namespace MiniDB::Engine {

// ============================================================
//  Section 1 — Constructor
// ============================================================
QueryEngine::QueryEngine(Arena<>& arena)   // BUGFIX: was ArenaAllocator&, mismatched the header
    : arena(arena) {}


// ============================================================
//  Section 2 — Core Query Operation
// ============================================================
QueryResult QueryEngine::select(
    Table&                              table,
    std::span<const FilterPredicate>    predicates,
    const SortCondition*                sort,
    std::size_t                         limit)
{
    QueryResult  result;
    ArenaScope   scope(arena);

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;

            result.records.push_back(*r);
            result.totalMatched++;

            if (limit > 0 && result.records.size() >= limit) {
                result.status = Status::OK;
                return finishSelect(std::move(result), sort);
            }
        }
    }

    result.status = Status::OK;
    return finishSelect(std::move(result), sort);
}

QueryResult QueryEngine::selectAll(Table& table) {
    return select(table, std::span<const FilterPredicate>{});
}

QueryResult QueryEngine::selectByID(Table& table, RecordID id) {
    QueryResult result;

    Record  out;
    Status  s = table.getRecord(id, out);

    if (s != Status::OK) {
        result.status = s;
        return result;
    }

    result.records.push_back(out);
    result.totalMatched  = 1;
    result.status        = Status::OK;
    return result;
}


// ============================================================
//  Section 3 — Aggregate
// ============================================================
std::size_t QueryEngine::count(
    Table&                              table,
    std::span<const FilterPredicate>    predicates)
{
    std::size_t  total = 0;
    ArenaScope   scope(arena);

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;
            if (evaluateAll(*r, predicates)) total++;
        }
    }
    return total;
}

double QueryEngine::sum(
    Table&                              table,
    const FieldName&                    field,
    std::span<const FilterPredicate>    predicates)
{
    double      total = 0.0;
    ArenaScope  scope(arena);

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;

            const Json& val = r->getFieldRef(field);   // was: getField() (copy)
            if (val.isNumber()) total += val.asNumber();
        }
    }
    return total;
}

double QueryEngine::avg(
    Table&                              table,
    const FieldName&                    field,
    std::span<const FilterPredicate>    predicates)
{
    double       total = 0.0;
    std::size_t  count = 0;
    ArenaScope   scope(arena);

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;

            const Json& val = r->getFieldRef(field);
            if (val.isNumber()) {
                total += val.asNumber();
                ++count;
            }
        }
    }
    return count > 0 ? total / static_cast<double>(count) : 0;
}

double QueryEngine::max(
    Table&                              table,
    const FieldName&                    field,
    std::span<const FilterPredicate>    predicates)
{
    double      result = 0.0;
    bool        found = false;
    ArenaScope  scope(arena);

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;

            const Json& val = r->getFieldRef(field);
            if (!val.isNumber()) continue;

            double num = val.asNumber();
            if (!found || num > result) {
                result = num;
                found = true;
            }
        }
    }
    return result;
}

double QueryEngine::min(
    Table&                              table,
    const FieldName&                    field,
    std::span<const FilterPredicate>    predicates)
{
    double      result = 0.0;
    bool        found = false;
    ArenaScope  scope(arena);

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;

            const Json& val = r->getFieldRef(field);
            if (!val.isNumber()) continue;

            double num = val.asNumber();
            if (!found || num < result) {
                result = num;
                found = true;
            }
        }
    }
    return result;
}


// ============================================================
//  Section 4 — Helpers
// ============================================================
QueryResult QueryEngine::finishSelect(QueryResult result, const SortCondition* sort) const {
    if (sort && result.records.size() > 1)
        sortResults(result.records, *sort);
    return result;
}

bool QueryEngine::evaluatePredicate(
    const Record&           record,
    const FilterPredicate&  predicate) const
{
    if (!record.hasField(predicate.field)) return false;

    const Json& val = record.getFieldRef(predicate.field);   // was: getField() (copy)
    return compareValues(val, predicate.value, predicate.op);
}

bool QueryEngine::evaluateAll(
    const Record&                      record,
    std::span<const FilterPredicate>   predicates) const
{
    // predicates are now values in a contiguous span: no pointer
    // indirection per predicate, no forced heap allocation by the caller
    // (was Vector<FilterPredicate*>).
    for (const FilterPredicate& p : predicates) {
        if (!evaluatePredicate(record, p)) return false;
    }
    return true;
}

void QueryEngine::sortResults(
    Vector<Record>&      records,
    const SortCondition&  sort) const
{
    // BUGFIX: previous hand-rolled bubble sort had an off-by-one inner bound
    // (`j < n - i` instead of `j < n - i - 1`), reading records[n] out of
    // bounds on every call -- undefined behavior. Replaced with std::sort:
    // correct, and O(n log n) instead of O(n^2).
    std::sort(records.begin(), records.end(),
        [this, &sort](const Record& a, const Record& b) {
            const Json& fa = a.getFieldRef(sort.field);
            const Json& fb = b.getFieldRef(sort.field);
            return sort.order == SortOrder::ASC
                 ? compareValues(fa, fb, Op::LT)
                 : compareValues(fb, fa, Op::LT);
        });
}

bool QueryEngine::compareValues(
    const Json&  a,
    const Json&  b,
    Op           op) const
{
    if (a.isString() && b.isString()) {
        const std::string& sa = a.asString();
        const std::string& sb = b.asString();
        switch (op) {
            case Op::EQ:   return sa == sb;
            case Op::NEQ:  return sa != sb;
            case Op::GT:   return sa > sb;
            case Op::GTE:  return sa >= sb;
            case Op::LT:   return sa < sb;
            case Op::LTE:  return sa <= sb;
        }
    }

    if (a.isNumber() && b.isNumber()) {
        double na = a.asNumber();
        double nb = b.asNumber();
        switch (op) {
            case Op::EQ:   return na == nb;
            case Op::NEQ:  return na != nb;
            case Op::GT:   return na > nb;
            case Op::GTE:  return na >= nb;
            case Op::LT:   return na < nb;
            case Op::LTE:  return na <= nb;
        }
    }

    if (a.isBool() && b.isBool()) {
        bool ba = a.asBool();
        bool bb = b.asBool();
        switch (op) {
            case Op::EQ:   return ba == bb;
            case Op::NEQ:  return ba != bb;
            default:       return false;
        }
    }

    return false;
}

} // namespace MiniDB::Engine
