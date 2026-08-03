// Aggregate Query Pipeline.
//
// Demonstrates:
// - chaining filter + sort + limit into one select() for browsing a slice
//   of data
// - running sum/avg/max/min as separate calls over the *same* predicates,
//   since QueryEngine has no single call that returns rows and aggregates
//   together

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static void run_examples() {
    setTitle("Seed Some Sales Data");

    Vector<ColumnDef> schema{
        ColumnDef{"region", ColumnType::STRING, false},
        ColumnDef{"revenue", ColumnType::DOUBLE, false},
    };
    Database db("app");
    (void)db.createTable("sales", schema);
    Table* sales = db.getTable("sales");

    struct Seed {
        RecordID id;
        const char* region;
        double revenue;
    };
    for (const Seed& s : {Seed{1, "west", 120.0}, Seed{2, "east", 80.0}, Seed{3, "west", 95.0},
                          Seed{4, "west", 300.0}, Seed{5, "east", 60.0}}) {
        Record r(s.id);
        (void)r.setField("region", Json(s.region));
        (void)r.setField("revenue", Json(s.revenue));
        (void)sales->insertRecord(r);
    }

    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine engine(arena);

    // The same FilterPredicate span is reused across every call below --
    // select() and the aggregate methods each independently apply it, so
    // "the pipeline" here means composing several calls over one
    // consistent filter, not one method chain.
    setTitle("Define The Pipeline's Filter");

    FilterPredicate westOnly{"region", Op::EQ, Json("west")};
    std::span<const FilterPredicate> preds(&westOnly, 1);

    // Step 1: browse the top 2 matching rows, sorted by revenue.
    setTitle("Step 1 -- Top 2 Rows (filter + sort + limit)");

    SortCondition byRevenueDesc{"revenue", SortOrder::DESC};
    QueryResult top2 = engine.select(*sales, preds, &byRevenueDesc, 2);
    for (const Record& r : top2.records) {
        std::cout << r.getField("region").asString() << " : " << r.getField("revenue").asNumber()
                  << "\n";
    }
    std::cout << "\n";

    // Step 2: aggregate over the *same* filter, independent of the sort
    // and limit used above -- aggregates always scan every matching row,
    // regardless of any limit applied in a separate select() call.
    setTitle("Step 2 -- Aggregates Over The Same Filter");

    std::cout << "count : " << engine.count(*sales, preds) << "\n";
    std::cout << "sum   : " << engine.sum(*sales, "revenue", preds) << "\n";
    std::cout << "avg   : " << engine.avg(*sales, "revenue", preds) << "\n";
    std::cout << "max   : " << engine.max(*sales, "revenue", preds) << "\n";
    std::cout << "min   : " << engine.min(*sales, "revenue", preds) << "\n";
}

REGISTER_EXAMPLE_SUITE();