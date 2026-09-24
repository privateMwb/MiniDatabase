// Table Test Suite
// Verifies CRUD, schema validation propagation, page allocation, and
// serialization for a single Table in isolation (no Database, no file I/O).
//
// Covers:
// - insertRecord / getRecord / updateRecord / deleteRecord
// - schema introspection (getSchema / hasColumn)
// - page allocation across the MAX_RECORDS_PAGE boundary
// - compact() / rebuildIndex()
// - toJson / fromJson round trip, including that the page-id counter
//   resumes above the highest loaded page id after a load (regression
//   coverage for the id-collision-after-reuse bug)
// - serialize / deserialize round trip
// - deserialize on malformed input

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"score", ColumnType::INT, true},
    };
}

Record makeRecord(RecordID id, const std::string& name) {
    Record r(id);
    (void)r.setField("name", Json(name));
    return r;
}

} // namespace

// Verifies basic construction and introspection getters.
static void construct_basic() {
    Table t("users", 1, makeSchema());

    CHK(t.getName() == "users");
    CHK(t.getID() == 1);
    CHK(t.isEmpty() == true);
    CHK(t.recordCount() == 0);
    CHK(t.pageCount() == 0);
}

// Verifies hasColumn against the schema passed at construction.
static void has_column_true_false() {
    Table t("users", 1, makeSchema());

    CHK(t.hasColumn("name") == true);
    CHK(t.hasColumn("score") == true);
    CHK(t.hasColumn("missing") == false);
}

// Verifies insertRecord followed by getRecord returns the same data.
static void insert_and_get() {
    Table t("users", 1, makeSchema());

    CHK(t.insertRecord(makeRecord(1, "Ada")) == Status::OK);

    Record out;
    CHK(t.getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");
}

// Verifies that inserting a record with an id already present fails with
// DUPLICATE_KEY rather than overwriting.
static void insert_duplicate_fails() {
    Table t("users", 1, makeSchema());

    CHK(t.insertRecord(makeRecord(1, "Ada")) == Status::OK);
    CHK(t.insertRecord(makeRecord(1, "Grace")) == Status::DUPLICATE_KEY);
}

// Verifies that schema validation failures from Record::validate propagate
// through insertRecord rather than being silently accepted.
static void insert_invalid_schema_fails() {
    Table t("users", 1, makeSchema());

    Record bad(1);
    (void)bad.setField("name", Json(123)); // STRING column holding a number

    CHK(t.insertRecord(bad) != Status::OK);
    CHK(t.recordCount() == 0);
}

// Verifies getRecord on a missing id returns NOT_FOUND.
static void get_missing_returns_not_found() {
    Table t("users", 1, makeSchema());

    Record out;
    CHK(t.getRecord(999, out) == Status::NOT_FOUND);
}

// Verifies updateRecord replaces existing data.
static void update_existing_replaces_data() {
    Table t("users", 1, makeSchema());
    CHK(t.insertRecord(makeRecord(1, "Ada")) == Status::OK);

    CHK(t.updateRecord(makeRecord(1, "Grace")) == Status::OK);

    Record out;
    CHK(t.getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Grace");
}

// Verifies updateRecord on a missing id returns NOT_FOUND.
static void update_missing_returns_not_found() {
    Table t("users", 1, makeSchema());
    CHK(t.updateRecord(makeRecord(1, "Ada")) == Status::NOT_FOUND);
}

// Verifies deleteRecord removes the record from the index (subsequent
// getRecord returns NOT_FOUND) and reports NOT_FOUND on a missing id.
static void delete_existing_and_missing() {
    Table t("users", 1, makeSchema());
    CHK(t.insertRecord(makeRecord(1, "Ada")) == Status::OK);

    CHK(t.deleteRecord(1) == Status::OK);

    Record out;
    CHK(t.getRecord(1, out) == Status::NOT_FOUND);
    CHK(t.deleteRecord(1) == Status::NOT_FOUND);
}

// Verifies that inserting more than MAX_RECORDS_PAGE records allocates a
// second page, and every record remains reachable via getRecord (exercises
// the O(1) PageID -> Page* index across multiple pages).
static void overflow_allocates_second_page() {
    Table t("users", 1, makeSchema());

    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
        CHK(t.insertRecord(makeRecord(i, "user_" + std::to_string(i))) == Status::OK);
    }

    CHK(t.pageCount() == 2);
    CHK(t.recordCount() == DBConstants::MAX_RECORDS_PAGE + 1);

    Record out;
    CHK(t.getRecord(0, out) == Status::OK);
    CHK(t.getRecord(DBConstants::MAX_RECORDS_PAGE, out) == Status::OK);
}

// Verifies compact() clears the dirty flag.
static void compact_clears_dirty() {
    Table t("users", 1, makeSchema());
    CHK(t.insertRecord(makeRecord(1, "Ada")) == Status::OK);
    CHK(t.isDirty() == true);

    CHK(t.compact() == Status::OK);
    CHK(t.isDirty() == false);
}

// Verifies rebuildIndex() reconstructs lookups correctly (records remain
// retrievable after an explicit rebuild).
static void rebuild_index_preserves_lookups() {
    Table t("users", 1, makeSchema());
    CHK(t.insertRecord(makeRecord(1, "Ada")) == Status::OK);
    CHK(t.insertRecord(makeRecord(2, "Grace")) == Status::OK);

    CHK(t.rebuildIndex() == Status::OK);

    Record out;
    CHK(t.getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");
    CHK(t.getRecord(2, out) == Status::OK);
    CHK(out.getField("name").asString() == "Grace");
}

// Verifies toJson()/fromJson() preserves schema and all records, and that
// getRecord works immediately after fromJson() without a separate
// rebuildIndex() call.
static void json_round_trip_preserves_data() {
    Table original("users", 1, makeSchema());
    CHK(original.insertRecord(makeRecord(1, "Ada")) == Status::OK);
    CHK(original.insertRecord(makeRecord(2, "Grace")) == Status::OK);

    Json envelope = original.toJson();

    Table restored("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(restored.fromJson(envelope) == Status::OK);

    CHK(restored.getName() == "users");
    CHK(restored.recordCount() == 2);

    Record out;
    CHK(restored.getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");
    CHK(restored.getRecord(2, out) == Status::OK);
    CHK(out.getField("name").asString() == "Grace");
}

// Regression: after loading a table via fromJson(), the page-id counter
// must resume above the highest loaded page id, so a newly-allocated page
// can never collide with one restored from disk.
static void page_id_resumes_after_load() {
    Table original("users", 1, makeSchema());
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
        CHK(original.insertRecord(makeRecord(i, "user_" + std::to_string(i))) == Status::OK);
    }
    CHK(original.pageCount() == 2);

    PageID maxLoadedPageId = 0;
    for (const Page* p : original.getPages()) {
        if (p->getID() > maxLoadedPageId)
            maxLoadedPageId = p->getID();
    }

    Json envelope = original.toJson();
    Table restored("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(restored.fromJson(envelope) == Status::OK);

    // Force allocation of a brand new page beyond what was loaded.
    for (RecordID i = 1000; i < 1000 + DBConstants::MAX_RECORDS_PAGE + 1; ++i) {
        CHK(restored.insertRecord(makeRecord(i, "extra")) == Status::OK);
    }

    bool sawNewPageIdBeyondLoaded = false;
    for (const Page* p : restored.getPages()) {
        if (p->getID() > maxLoadedPageId)
            sawNewPageIdBeyondLoaded = true;
    }
    CHK(sawNewPageIdBeyondLoaded == true);
}

// Verifies serialize()/deserialize() round trip matches toJson()/fromJson().
static void serialize_round_trip() {
    Table original("users", 1, makeSchema());
    CHK(original.insertRecord(makeRecord(1, "Ada")) == Status::OK);

    std::string raw = original.serialize();

    Table restored("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(restored.deserialize(raw) == Status::OK);

    CHK(restored.getName() == "users");
    Record out;
    CHK(restored.getRecord(1, out) == Status::OK);
    CHK(out.getField("name").asString() == "Ada");
}

// Verifies deserialize() on malformed JSON returns PARSE_ERROR.
static void deserialize_malformed_returns_parse_error() {
    Table t("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(t.deserialize("{not valid json") == Status::PARSE_ERROR);
}

// Executes all Table test cases.
static void run_tests() {
    RUN(construct_basic);
    RUN(has_column_true_false);
    RUN(insert_and_get);
    RUN(insert_duplicate_fails);
    RUN(insert_invalid_schema_fails);
    RUN(get_missing_returns_not_found);
    RUN(update_existing_replaces_data);
    RUN(update_missing_returns_not_found);
    RUN(delete_existing_and_missing);
    RUN(overflow_allocates_second_page);
    RUN(compact_clears_dirty);
    RUN(rebuild_index_preserves_lookups);
    RUN(json_round_trip_preserves_data);
    RUN(page_id_resumes_after_load);
    RUN(serialize_round_trip);
    RUN(deserialize_malformed_returns_parse_error);
}

REGISTER_TEST_SUITE();
