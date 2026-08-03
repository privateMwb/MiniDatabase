// Pagination.
//
// Demonstrates:
// - QueryEngine only supports `limit` (a page size), not an offset -- so
//   paging through results means keyset ("cursor") pagination: sort by a
//   field, take a page, remember the last value seen, then filter for
//   "greater than that value" on the next call
// - that RecordID itself is NOT queryable through FilterPredicate --
//   predicates only look at fields inside a Record's data, so a field you
//   want to paginate or filter by must be stored explicitly in the schema,
//   even if it duplicates information already implied by the RecordID

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static Table makeLog() {
    // "seq" is stored explicitly as a data field so it can be used in a
    // FilterPredicate. RecordID (the id passed to Record's constructor)
    // cannot be referenced by a predicate -- only fields set via
    // setField() can.
    Vector<ColumnDef> schema{
        ColumnDef{"seq", ColumnType::INT, false},
        ColumnDef{"message", ColumnType::STRING, false},
    };
    Table log("log", 1, schema);

    for (int i = 0; i < 12; ++i) {
        Record r(static_cast<RecordID>(i));
        (void)r.setField("seq", Json(i));
        (void)r.setField("message", Json("entry_" + std::to_string(i)));
        (void)log.insertRecord(r);
    }
    return log;
}

static void printPage(const QueryResult& page) {
    for (const Record& r : page.records) {
        std::cout << "  seq " << r.getField("seq").asNumber() << " : "
                  << r.getField("message").asString() << "\n";
    }
}

static void run_examples() {
    Arena<> arena(DBConstants::ARENA_SIZE);
    QueryEngine qe(arena);
    Table log = makeLog();

    constexpr std::size_t pageSize = 5;
    SortCondition bySeqAsc{"seq", SortOrder::ASC};

    // First page: no lower bound yet, just sort + limit.
    setTitle("Page 1");

    QueryResult page1 = qe.select(log, std::span<const FilterPredicate>{}, &bySeqAsc, pageSize);
    printPage(page1);

    double lastSeq = page1.records.empty() ? -1.0 : page1.records.back().getField("seq").asNumber();
    std::cout << "  (last seq on this page: " << lastSeq << ")\n\n";

    // Second page: filter for seq > lastSeq, keep the same sort + limit.
    setTitle("Page 2");

    FilterPredicate afterPage1{"seq", Op::GT, Json(lastSeq)};
    QueryResult page2 =
        qe.select(log, std::span<const FilterPredicate>(&afterPage1, 1), &bySeqAsc, pageSize);
    printPage(page2);

    lastSeq = page2.records.empty() ? lastSeq : page2.records.back().getField("seq").asNumber();
    std::cout << "  (last seq on this page: " << lastSeq << ")\n\n";

    // Third page: same pattern. With 12 total records and a page size of
    // 5, this final page only has 2 records left.
    setTitle("Page 3 (Partial)");

    FilterPredicate afterPage2{"seq", Op::GT, Json(lastSeq)};
    QueryResult page3 =
        qe.select(log, std::span<const FilterPredicate>(&afterPage2, 1), &bySeqAsc, pageSize);
    printPage(page3);
    std::cout << "  (" << page3.records.size()
              << " record(s) -- fewer than pageSize means this was the last page)\n";
}

REGISTER_EXAMPLE_SUITE();
