// Table Page-ID Collision Regression Test
// Pins the fix where page ids were sourced from `pages.size()` instead of
// a monotonic counter resumed from (max loaded id + 1) on load.
//
// Covers:
// - a new page allocated after loading a table continues the id sequence
//   from the highest loaded page id, not from pages.size()
// - the id sequence stays unique and monotonic across multiple subsequent
//   allocations after a reload, not just the first one

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static Table loadTableWithOneFullPage(PageID seedId) {
    Page seedPage(seedId);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        (void)seedPage.addRecord(Record(i));
    }
    Json pageArr(Json::ArrayType{});
    pageArr.asArray().push_back(seedPage.toJson());

    Json envelope(Json::ObjectType{});
    envelope["__table_id___"] = 1;
    envelope["__name__"] = "orders";
    envelope["schema"] = Json(Json::ArrayType{});
    envelope["page"] = pageArr;

    Table t("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    (void)t.fromJson(envelope);
    return t;
}

// Verifies that after loading a table whose single page has id 100 (NOT
// equal to pages.size(), which is 1), inserting enough records to force a
// new page allocates id 101 -- continuing from the loaded max, not from
// pages.size() (which would have produced the colliding id 1 under the
// pre-fix behavior).
static void table_reload_new_page_id_unique() {
    Table t = loadTableWithOneFullPage(100);
    CHK(t.pageCount() == 1);

    CHK(t.insertRecord(Record(9999)) == Status::OK);
    CHK(t.pageCount() == 2);

    PageID newId = DBConstants::INVALID_PAGE_ID;
    for (const Page* p : t.getPages()) {
        if (p->getID() != 100)
            newId = p->getID();
    }
    CHK(newId == 101);
    CHK(newId != 1);
}

// Verifies the id sequence stays unique and monotonic across MULTIPLE
// subsequent page allocations, not just the first one after a reload --
// pages.size()-sourced ids would repeat once size() cycles back through
// small numbers as more pages accumulate.
static void table_reload_multiple_pages_unique_ids() {
    Table t = loadTableWithOneFullPage(50);

    RecordID nextRecordId = 100000;
    for (int fillCount = 0; fillCount < 2; ++fillCount) {
        for (std::uint32_t i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
            CHK(t.insertRecord(Record(nextRecordId++)) == Status::OK);
        }
    }
    CHK(t.pageCount() == 3);

    bool has50 = false, has51 = false, has52 = false;
    for (const Page* p : t.getPages()) {
        if (p->getID() == 50)
            has50 = true;
        if (p->getID() == 51)
            has51 = true;
        if (p->getID() == 52)
            has52 = true;
    }
    CHK(has50 && has51 && has52);
}

static void run_tests() {
    RUN(table_reload_new_page_id_unique);
    RUN(table_reload_multiple_pages_unique_ids);
}

REGISTER_TEST_SUITE();