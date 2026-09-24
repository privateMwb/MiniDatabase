// Database Load Atomicity Regression Test
// Pins Database::fromJson's scratch-then-swap behavior: tables are parsed
// into scratch containers and only swapped into `*this` once every table
// parses successfully.
//
// Covers:
// - a mid-load failure leaves name/tables completely untouched, with no
//   partially-applied name change and no partially-loaded table leaking in
// - a failed load doesn't corrupt the table-id counter or the dirty flag

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

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
TEST(DatabaseLoadAtomicity, LoadFailureStateUntouched) {
    Database db("shop");
    ASSERT_EQ(db.createTable("orders", Vector<ColumnDef>{}), Status::OK);
    ASSERT_EQ(db.createTable("customers", Vector<ColumnDef>{}), Status::OK);

    ASSERT_EQ(db.fromJson(makeBadEnvelope()), Status::PARSE_ERROR);

    EXPECT_EQ(db.getName(), "shop");
    EXPECT_EQ(db.tableCount(), 2);
    EXPECT_TRUE(db.hasTable("orders"));
    EXPECT_TRUE(db.hasTable("customers"));
    EXPECT_FALSE(db.hasTable("new_table"));
}

// Verifies a failed load doesn't corrupt the id counter or dirty flag
// either -- only a *successful* fromJson is allowed to touch them.
TEST(DatabaseLoadAtomicity, LoadFailurePreservesCounters) {
    Database db("shop");
    ASSERT_EQ(db.createTable("orders", Vector<ColumnDef>{}), Status::OK); // id 0
    ASSERT_TRUE(db.isDirty());

    ASSERT_EQ(db.fromJson(makeBadEnvelope()), Status::PARSE_ERROR);
    EXPECT_TRUE(db.isDirty()); // untouched by the failed attempt

    ASSERT_EQ(db.createTable("customers", Vector<ColumnDef>{}), Status::OK);
    EXPECT_EQ(db.getTable("customers")->getID(), 1); // counter wasn't corrupted
}
