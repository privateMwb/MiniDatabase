// Database Table-ID Collision Regression Test
// Pins the fix where table ids were sourced from `tables.size()` instead
// of a monotonic counter resumed from (max loaded id + 1) on load, and
// which shrinks (and therefore collides) on dropTable().
//
// Covers:
// - a new table created after loading a database continues the id
//   sequence from the highest loaded table id, not from tables.size()
// - the classic create/drop/create cycle does not hand a dropped table's
//   vacated id back out to a table created afterward

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Verifies that after loading a database whose single table has id 100
// (NOT equal to tables.size(), which is 1), creating a new table allocates
// id 101 -- not the colliding id 1 a tables.size()-sourced counter would
// have produced.
static void database_reload_new_table_id_unique() {
    Json tableJson(Json::ObjectType{});
    tableJson["__table_id___"] = 100;
    tableJson["__name__"] = "orders";
    tableJson["schema"] = Json(Json::ArrayType{});
    tableJson["page"] = Json(Json::ArrayType{});

    Json tableArr(Json::ArrayType{});
    tableArr.asArray().push_back(tableJson);

    Json envelope(Json::ObjectType{});
    envelope["__db_name__"] = "shop";
    envelope["tables"] = tableArr;

    Database db("shop");
    CHK(db.fromJson(envelope) == Status::OK);
    CHK(db.getTable("orders")->getID() == 100);

    CHK(db.createTable("customers", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.getTable("customers")->getID() == 101);
}

// Verifies the classic create/drop/create cycle: dropping a table shrinks
// tables.size(), and a size()-sourced id source would then hand the
// vacated id straight back out to the next createTable(), even though
// another live table may already be using it.
static void database_create_drop_create_no_reuse() {
    Database db("shop");
    CHK(db.createTable("a", Vector<ColumnDef>{}) == Status::OK); // id 0
    CHK(db.createTable("b", Vector<ColumnDef>{}) == Status::OK); // id 1
    CHK(db.dropTable("a") == Status::OK);
    CHK(db.createTable("c", Vector<ColumnDef>{}) == Status::OK); // must be 2

    CHK(db.getTable("b")->getID() == 1);
    CHK(db.getTable("c")->getID() == 2);
}

static void run_tests() {
    RUN(database_reload_new_table_id_unique);
    RUN(database_create_drop_create_no_reuse);
}

REGISTER_TEST_SUITE();