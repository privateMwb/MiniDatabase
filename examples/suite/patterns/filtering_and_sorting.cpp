// Filtering and Sorting.
//
// Demonstrates:
// - a single FilterPredicate passed to select()
// - combining multiple predicates (AND semantics)
// - sorting results ascending and descending with SortCondition
// - selectAll() and an empty predicate span meaning "no filter"

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static Table makeCatalog() {
    Vector<ColumnDef> schema{
        ColumnDef{"title", ColumnType::STRING, false},
        ColumnDef{"price", ColumnType::DOUBLE, false},
        ColumnDef{"tag", ColumnType::STRING, false},
    };
    Table catalog("catalog", 1, schema);

    struct Item {
        const char* title;
        double price;
        const char* tag;
    };
    Item items[] = {
        {"Widget", 9.99, "hardware"},      {"Gadget", 19.99, "electronics"},
        {"Gizmo", 4.99, "hardware"},       {"Doohickey", 14.99, "electronics"},
        {"Thingamajig", 2.99, "hardware"},
    };

    RecordID id = 1;
    for (const auto& item : items) {
        Record r(id++);
        (void)r.setField("title", Json(item.title));
        (void)r.setField("price", Json(item.price));
        (void)r.setField("tag", Json(item.tag));
        (void)catalog.insertRecord(r);
    }
    return catalog;
}

static void printTitles(const QueryResult& result) {
    for (const Record& r : result.records) {
        std::cout << "  " << r.getField("title").asString() << " ($"
                  << r.getField("price").asNumber() << ")\n";
    }
}

static void run_examples() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table catalog = makeCatalog();

    // No filter at all: selectAll(), or select() with an empty span --
    // both mean the same thing.
    setTitle("No Filter");

    QueryResult all = qe.selectAll(catalog);
    std::cout << "All items (" << all.records.size() << "):\n";
    printTitles(all);
    std::cout << "\n";

    // A single predicate: price > 10.
    setTitle("Single Predicate");

    FilterPredicate expensive{"price", Op::GT, Json(10.0)};
    QueryResult pricey = qe.select(catalog, std::span<const FilterPredicate>(&expensive, 1));
    std::cout << "Items over $10 (" << pricey.records.size() << "):\n";
    printTitles(pricey);
    std::cout << "\n";

    // Multiple predicates are combined with AND: every predicate must
    // match for a record to be included.
    setTitle("Multiple Predicates (AND)");

    FilterPredicate predicates[] = {
        {"tag", Op::EQ, Json("hardware")},
        {"price", Op::LT, Json(5.0)},
    };
    QueryResult cheapHardware = qe.select(catalog, std::span<const FilterPredicate>(predicates, 2));
    std::cout << "Hardware under $5 (" << cheapHardware.records.size() << "):\n";
    printTitles(cheapHardware);
    std::cout << "\n";

    // Sorting: pass a SortCondition as the third argument. Works with or
    // without a filter.
    setTitle("Sorting Ascending");

    SortCondition byPriceAsc{"price", SortOrder::ASC};
    QueryResult ascending = qe.select(catalog, std::span<const FilterPredicate>{}, &byPriceAsc);
    std::cout << "All items, cheapest first:\n";
    printTitles(ascending);
    std::cout << "\n";

    setTitle("Sorting Descending");

    SortCondition byPriceDesc{"price", SortOrder::DESC};
    QueryResult descending = qe.select(catalog, std::span<const FilterPredicate>{}, &byPriceDesc);
    std::cout << "All items, priciest first:\n";
    printTitles(descending);
}

REGISTER_EXAMPLE_SUITE();
