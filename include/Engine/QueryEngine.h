#pragma once

#include <string>
#include <functional>

#include "../Common/Type.h"
#include "../Core/Record.h"
#include "../Core/Table.h"

#include "../../libs/VectorPro/VectorPro.h"
#include "../../libs/JsonParser/Json.h"
#include "../../libs/CustomAllocators/ArenaAllocator/ArenaAllocator.h"
#include "../../libs/CustomAllocators/ArenaAllocator/ArenaScope.h"

// Filter Predicate
struct FilterPredicate {
	FieldName  field;
	Op         op;
	Json       value;
};

// Sort Condition
struct SortCondition {
	FieldName  field;
	SortOrder  order;
};

// Query Result
struct QueryResult {
	VectorPro<Record>  records;
	std::size_t        totalMatched = 0;
	Status             status = Status::OK;
};

class QueryEngine {
    private:
    
    // Arena Reference
    ArenaAllocator& arena;
    
public:

	// Constructor
	explicit QueryEngine(ArenaAllocator& arena);

	// Core Query Operation
	[[nodiscard]] QueryResult select(
	    Table&                              table,
	    const VectorPro<FilterPredicate*>&   predicates,
	    const SortCondition*                sort = nullptr,
	    std::size_t                         limit = 0
	);

	[[nodiscard]] QueryResult selectAll(Table& table);
	[[nodiscard]] QueryResult selectByID(Table& table, RecordID id);

	// Aggregate
	[[nodiscard]] std::size_t count(
	    Table&                              table,
	    const VectorPro<FilterPredicate*>&  predicates
	);

    [[nodiscard]] double sum(
	    Table&                              table,
	    const FieldName&                    field,
	    const VectorPro<FilterPredicate*>&  predicates
	);
	
	[[nodiscard]] double avg(
	    Table&                              table,
	    const FieldName&                    field,
	    const VectorPro<FilterPredicate*>&  predicates
	);
	
	[[nodiscard]] double max(
	    Table&                              table,
	    const FieldName&                    field,
	    const VectorPro<FilterPredicate*>&  predicates
	);

    [[nodiscard]] double min(
	    Table&                              table,
	    const FieldName&                    field,
	    const VectorPro<FilterPredicate*>&  predicates
	);

private:

    // Helpers
    [[nodiscard]] bool evaluatePredicate(
        const Record&           record,
        const FilterPredicate&  predicate
    ) const;
    
    [[nodiscard]] bool evaluateAll(
        const Record&                       record,
        const VectorPro<FilterPredicate*>&  predicates
    ) const;

    void sortResults(
        VectorPro<Record>&    records,
        const SortCondition&  sort
    ) const;
    
    [[nodiscard]] bool compareValues(
        const Json&  a,
        const Json&  b,
        Op           op
    ) const;
};





