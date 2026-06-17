#include <stdexcept>

#include "../../include/Engine/QueryEngine.h"

// Constructor
QueryEngine::QueryEngine(ArenaAllocator& arena)
	: arena(arena) {}

// Core Query Operation
QueryResult QueryEngine::select(
    Table&                              table,
    const VectorPro<FilterPredicate*>&  predicates,
    const SortCondition*                sort,
    std::size_t                         limit)
{
    QueryResult  result;
    ArenaScope   scope(arena);
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            
            result.records.push_back(*r);
            result.totalMatched++;
            
            if (limit > 0 && result.records.size() >= limit) {
                result.status = Status::OK;
                goto done;
            }
        }
    }
    
    done:
    
    if(sort && result.records.size() > 1)
        sortResults(result.records, *sort);
    
    result.status = Status::OK;
    return result;
}

QueryResult QueryEngine::selectAll(Table& table) {
    return select(table, VectorPro<FilterPredicate*>{});
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

// Aggregate
std::size_t QueryEngine::count(
    Table&                              table,
    const VectorPro<FilterPredicate*>&  predicates) 
{
    std::size_t  total = 0;
    ArenaScope   scope(arena);
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            if (evaluateAll(*r, predicates)) total++;
        }
    }
    return total;
}

double QueryEngine::sum(
    Table&                              table,
    const FieldName&                    field,
    const VectorPro<FilterPredicate*>&  predicates)
{
    double      total = 0.0;
    ArenaScope  scope(arena);
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;
            
            Json val = r->getField(field);
            if (val.isNumber()) total += val.asNumber();
        }
    }
    return total;
}

double QueryEngine::avg(
    Table&                              table,
    const FieldName&                    field,
    const VectorPro<FilterPredicate*>&  predicates)
{
    double       total = 0.0;
    std::size_t  count = 0;
    ArenaScope   scope(arena);
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;
            
            Json val = r->getField(field);
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
    const VectorPro<FilterPredicate*>&  predicates)
{
    double      result = 0.0;
    bool        found = false;
    ArenaScope  scope(arena);
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;
            
            Json val = r->getField(field);
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
    const VectorPro<FilterPredicate*>&  predicates)
{
    double      result = 0.0;
    bool        found = false;
    ArenaScope  scope(arena);
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            if (!evaluateAll(*r, predicates)) continue;
            if (!r->hasField(field)) continue;
            
            Json val = r->getField(field);
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

// Helpers
bool QueryEngine::evaluatePredicate(
    const Record&           record,
    const FilterPredicate&  predicate) const
{
    if (!record.hasField(predicate.field)) return false;
    
    Json val = record.getField(predicate.field);
    return compareValues(val, predicate.value, predicate.op);
}

bool QueryEngine::evaluateAll(
    const Record&                      record,
    const VectorPro<FilterPredicate*>&  predicates) const 
{
    for (const FilterPredicate* p : predicates) {
        if (!evaluatePredicate(record, *p)) return false;
    }
    return true;
}

void QueryEngine::sortResults(
    VectorPro<Record>&    records,
    const SortCondition&  sort) const 
{
    std::size_t n = records.size();
    for (std::size_t i = 0; i < n - 1; ++i) {
        for (std::size_t j = 0; j < n - i; ++j) {
            Json a = records[j].getField(sort.field);
            Json b = records[j + 1].getField(sort.field);
            
            bool shouldSwap = false;
            
            if (sort.order == SortOrder::ASC) 
                shouldSwap = compareValues(b, a, Op::LT);
            else 
                shouldSwap = compareValues(a, b, Op::LT);
            
            if (shouldSwap) {
                Record tmp = std::move(records[j]);
                records[j] = std::move(records[j + 1]);
                records[j + 1] = std::move(tmp);
            }
        }
    }
}

bool QueryEngine::compareValues(
    const Json&  a,
    const Json&  b,
    Op           op) const
{
    if (a.isString() && b.isString()) {
        std::string sa = a.asString();
        std::string sb = b.asString();
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
