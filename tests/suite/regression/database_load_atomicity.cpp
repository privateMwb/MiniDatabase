// Database Load Atomicity Regression Test
// Pins Database::fromJson's scratch-then-swap behavior: tables are parsed
// into scratch containers and only swapped into `*this` once every table
// parses successfully.
//
// Covers:
// - a mid-load failure leaves name/tables completely untouched, with no
//   partially-applied name change and no partially-loaded table leaking in
// - a failed load doesn't corrupt the table-id counter or the dirty flag

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static Json makeBadEnvelope() {
    Json goodTable(Json::ObjectType{});
    goodTable["__table_id___"] = 0;
    goodTable["__name__"] = "new_table";
    goodTable["schema"] = Json(Json::ArrayType{});
    goodTable["page"] = Json(Json::ArrayType{});

    Json tableArr(Json::ArrayType{});
    tableArr.asArray().push_back(goodTable);
    tableArr.asArray().push_back(Json{}); // null -> Table::fromJson fails here

    Json envelope(Json::ObjectType{});
    envelope["__db_name__"] = "corrupt";
    envelope["tables"] = tableArr;
    return envelope;
}

// Verifies a mid-load failure leaves the pre-existing name/tables
// completely untouched -- no partially-applied name change, no
// partially-loaded table leaking in.
static void database_load_failure_state_untouched() {
    Database db("shop");
    CHK(db.createTable("orders", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("customers", Vector<ColumnDef>{}) == Status::OK);

    CHK(db.fromJson(makeBadEnvelope()) == Status::PARSE_ERROR);

    CHK(db.getName() == "shop");
    CHK(db.tableCount() == 2);
    CHK(db.hasTable("orders"));
    CHK(db.hasTable("customers"));
    CHK(!db.hasTable("new_table"));
}

// Verifies a failed load doesn't corrupt the id counter or dirty flag
// either -- only a *successful* fromJson is allowed to touch them.
static void database_load_failure_preserves_counters() {
    Database db("shop");
    CHK(db.createTable("orders", Vector<ColumnDef>{}) == Status::OK); // id 0
    CHK(db.isDirty());

    CHK(db.fromJson(makeBadEnvelope()) == Status::PARSE_ERROR);
    CHK(db.isDirty()); // untouched by the failed attempt

    CHK(db.createTable("customers", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.getTable("customers")->getID() == 1); // counter wasn't corrupted
}

static void run_tests() {
    RUN(database_load_failure_state_untouched);
    RUN(database_load_failure_preserves_counters);
}

REGISTER_TEST_SUITE();