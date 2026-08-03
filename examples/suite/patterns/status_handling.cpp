// Status Handling.
//
// Demonstrates:
// - MiniDB reports failure through a [[nodiscard]] Status return, not
//   exceptions -- every fallible call must be checked
// - the idiomatic "early return on first failure" pattern for chaining
//   several Status-returning calls
// - that [[nodiscard]] means the compiler itself will warn if a Status is
//   silently discarded -- `(void)` is the explicit way to say "I really do
//   mean to ignore this one"

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// A small helper that chains three fallible operations: create a table,
// build a record, insert it. Each step can fail independently, and this
// function stops at the first failure rather than pressing on with a
// database that isn't in the state the caller expects.
static Status seedOneOrder(Database& db) {
    Status s = db.createTable("orders", Vector<ColumnDef>{
                                            ColumnDef{"item", ColumnType::STRING, false},
                                        });
    if (s != Status::OK)
        return s;

    Table* orders = db.getTable("orders");
    if (!orders)
        return Status::TABLE_NOT_FOUND; // defensive: shouldn't happen right after createTable

    Record r(1);
    s = r.setField("item", Json("widget"));
    if (s != Status::OK)
        return s;

    return orders->insertRecord(r);
}

static void run_examples() {
    // The happy path: every step succeeds, and the chain returns OK.
    setTitle("Chained Calls, Happy Path");

    Database db("shop");
    Status s = seedOneOrder(db);
    std::cout << "seedOneOrder status : " << static_cast<int>(s) << " (OK)\n\n";

    // The chain stops at the first failure. Calling it again on the same
    // Database fails at createTable (the table already exists) --
    // everything after that line in seedOneOrder never runs.
    setTitle("Chained Calls, Failure Stops the Chain");

    s = seedOneOrder(db);
    std::cout << "seedOneOrder status (table already exists) : " << static_cast<int>(s)
              << " (TABLE_ALREADY_EXISTS)\n\n";

    // [[nodiscard]] Status means the compiler flags a call whose result is
    // silently thrown away. When you genuinely don't care about a
    // particular result -- often true for a setField() you already know
    // will succeed, or a best-effort cleanup step -- `(void)` documents
    // that the discard is intentional rather than an oversight.
    setTitle("Explicitly Discarding a Status");

    Record scratch(2);
    (void)scratch.setField("item", Json("gadget")); // intentional: not checked
    std::cout << "field set (result intentionally discarded)\n";
}

REGISTER_EXAMPLE_SUITE();
