// Page Test Suite
// Verifies record storage, soft-delete/compact lifecycle, and serialization
// for a single Page in isolation (no Table/Database, no file I/O).
//
// Covers:
// - construction (default vs explicit id) and initial empty state
// - addRecord: success, dirty flag, OUT_OF_MEMORY once full
// - getRecord (id-based, skips deleted) vs getRecordAt (index-based,
//   includes deleted)
// - updateRecord: existing, missing, and soft-deleted-then-update
// - deleteRecord: soft delete semantics (slot stays until compact())
// - compact(): removes deleted records, frees slots, clears dirty
// - toJson/fromJson round trip, excluding soft-deleted records
// - serialize/deserialize round trip
// - deserialize on malformed input
// - move construction/assignment

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Verifies the default constructor leaves the page with an invalid id and
// empty, clean state.
static void default_constructor_has_invalid_id() {
    Page p;
    CHK(p.getID() == DBConstants::INVALID_PAGE_ID);
    CHK(p.isEmpty());
    CHK(!p.isDirty());
}

// Verifies the explicit-id constructor sets id and starts empty/clean with
// a full complement of free slots.
static void explicit_constructor_sets_id_and_empty_state() {
    Page p(42);
    CHK(p.getID() == 42);
    CHK(p.isEmpty());
    CHK(p.recordCount() == 0);
    CHK(p.freeSlots() == DBConstants::MAX_RECORDS_PAGE);
    CHK(!p.isDirty());
}

// Verifies addRecord increases the record count and marks the page dirty.
static void add_record_increases_count_and_marks_dirty() {
    Page p(1);
    CHK(p.addRecord(Record(10)) == Status::OK);

    CHK(p.recordCount() == 1);
    CHK(!p.isEmpty());
    CHK(p.isDirty());
}

// Verifies that once a page holds MAX_RECORDS_PAGE records it reports full
// and rejects further inserts with OUT_OF_MEMORY.
static void add_record_fills_page_then_rejects() {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        CHK(p.addRecord(Record(i)) == Status::OK);
    }

    CHK(p.isFull());
    CHK(p.freeSlots() == 0);
    CHK(p.addRecord(Record(9999)) == Status::OUT_OF_MEMORY);
}

// Verifies getRecord finds a record by id.
static void get_record_returns_matching_record() {
    Page p(1);
    CHK(p.addRecord(Record(5)) == Status::OK);

    Record* got = p.getRecord(5);
    CHK(got != nullptr);
    CHK(got->getID() == 5);
}

// Verifies getRecord on a missing id returns nullptr rather than crashing.
static void get_record_missing_returns_nullptr() {
    Page p(1);
    CHK(p.getRecord(123) == nullptr);
}

// Verifies getRecord treats a soft-deleted record as not found.
static void get_record_skips_deleted() {
    Page p(1);
    CHK(p.addRecord(Record(1)) == Status::OK);
    CHK(p.deleteRecord(1) == Status::OK);

    CHK(p.getRecord(1) == nullptr);
}

// Verifies getRecordAt returns records by position and nullptr past the end.
static void get_record_at_by_index() {
    Page p(1);
    CHK(p.addRecord(Record(7)) == Status::OK);
    CHK(p.addRecord(Record(8)) == Status::OK);

    Record* first = p.getRecordAt(0);
    CHK(first != nullptr);
    CHK(first->getID() == 7);

    Record* second = p.getRecordAt(1);
    CHK(second != nullptr);
    CHK(second->getID() == 8);

    CHK(p.getRecordAt(2) == nullptr);
}

// Verifies getRecordAt, unlike getRecord(id), does NOT filter out
// soft-deleted records -- it's a raw positional accessor. Callers that need
// live-only records (e.g. QueryEngine's scan loop) must check isDeleted().
static void get_record_at_includes_deleted() {
    Page p(1);
    CHK(p.addRecord(Record(1)) == Status::OK);
    CHK(p.deleteRecord(1) == Status::OK);

    Record* r = p.getRecordAt(0);
    CHK(r != nullptr);
    CHK(r->isDeleted());
}

// Verifies updateRecord replaces the stored data for an existing record.
static void update_record_existing_replaces_data() {
    Page p(1);
    Record original(1);
    CHK(original.setField("x", Json(1)) == Status::OK);
    CHK(p.addRecord(original) == Status::OK);

    Record updated(1);
    CHK(updated.setField("x", Json(2)) == Status::OK);
    CHK(p.updateRecord(updated) == Status::OK);

    Record* got = p.getRecord(1);
    CHK(got != nullptr);
    CHK(got->getField("x").asNumber() == 2);
}

// Verifies updateRecord on a missing id reports NOT_FOUND.
static void update_record_missing_returns_not_found() {
    Page p(1);
    CHK(p.updateRecord(Record(555)) == Status::NOT_FOUND);
}

// Verifies updateRecord treats a soft-deleted record as not found (it goes
// through the same lookup as getRecord).
static void update_record_deleted_returns_not_found() {
    Page p(1);
    CHK(p.addRecord(Record(1)) == Status::OK);
    CHK(p.deleteRecord(1) == Status::OK);

    CHK(p.updateRecord(Record(1)) == Status::NOT_FOUND);
}

// Verifies deleteRecord soft-deletes: the record becomes unreachable via
// getRecord but still occupies a slot (recordCount unchanged) until
// compact() runs.
static void delete_record_marks_deleted_and_dirty() {
    Page p(1);
    CHK(p.addRecord(Record(1)) == Status::OK);
    CHK(p.deleteRecord(1) == Status::OK);

    CHK(p.getRecord(1) == nullptr);
    CHK(p.isDirty());
    CHK(p.recordCount() == 1);
}

// Verifies deleteRecord on a missing id reports NOT_FOUND.
static void delete_record_missing_returns_not_found() {
    Page p(1);
    CHK(p.deleteRecord(999) == Status::NOT_FOUND);
}

// Verifies compact() drops soft-deleted records, keeps live ones, and
// clears the dirty flag.
static void compact_removes_deleted_records() {
    Page p(1);
    CHK(p.addRecord(Record(1)) == Status::OK);
    CHK(p.addRecord(Record(2)) == Status::OK);
    CHK(p.deleteRecord(1) == Status::OK);

    CHK(p.compact() == Status::OK);

    CHK(p.recordCount() == 1);
    CHK(!p.isDirty());
    CHK(p.getRecord(2) != nullptr);
    CHK(p.getRecord(1) == nullptr);
}

// Verifies compact() reclaims slots occupied by soft-deleted records, so a
// full page becomes not-full again after compacting.
static void compact_frees_slots_for_reuse() {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        CHK(p.addRecord(Record(i)) == Status::OK);
    }
    CHK(p.isFull());

    CHK(p.deleteRecord(0) == Status::OK);
    CHK(p.isFull()); // soft-deleted slot not reclaimed yet

    CHK(p.compact() == Status::OK);
    CHK(!p.isFull());
    CHK(p.freeSlots() == 1);
}

// Verifies toJson excludes soft-deleted records and preserves the page id.
static void to_json_excludes_deleted_records() {
    Page p(1);
    CHK(p.addRecord(Record(10)) == Status::OK);
    CHK(p.addRecord(Record(20)) == Status::OK);
    CHK(p.deleteRecord(20) == Status::OK);

    Json envelope = p.toJson();
    CHK(envelope["__page_id__"].asNumber() == 1);

    const Json::ArrayType& arr = envelope["records"].asArray();
    CHK(arr.size() == 1);

    bool foundTen = false;
    for (const Json& entry : arr) {
        if (static_cast<RecordID>(entry["__id__"].asNumber()) == 10)
            foundTen = true;
    }
    CHK(foundTen);
}

// Verifies toJson()/fromJson() preserves id and live record data. The
// soft-deleted record is intentionally excluded from toJson, so it will
// not reappear after the round trip -- that's expected, not a bug.
static void json_round_trip_preserves_live_records() {
    Page original(4);
    Record r1(1);
    CHK(r1.setField("name", Json("Ada")) == Status::OK);
    Record r2(2);
    CHK(r2.setField("name", Json("Grace")) == Status::OK);

    CHK(original.addRecord(r1) == Status::OK);
    CHK(original.addRecord(r2) == Status::OK);
    CHK(original.deleteRecord(2) == Status::OK);

    Json envelope = original.toJson();

    Page restored;
    CHK(restored.fromJson(envelope) == Status::OK);

    CHK(restored.getID() == 4);
    CHK(restored.recordCount() == 1);
    CHK(!restored.isDirty());

    Record* got = restored.getRecord(1);
    CHK(got != nullptr);
    CHK(got->getField("name").asString() == "Ada");
}

// Verifies serialize()/deserialize() round trip matches toJson()/fromJson().
static void serialize_round_trip() {
    Page original(2);
    Record r(1);
    CHK(r.setField("v", Json(99)) == Status::OK);
    CHK(original.addRecord(r) == Status::OK);

    std::string raw = original.serialize();

    Page restored;
    CHK(restored.deserialize(raw) == Status::OK);

    CHK(restored.getID() == 2);
    CHK(restored.recordCount() == 1);

    Record* got = restored.getRecord(1);
    CHK(got != nullptr);
    CHK(got->getField("v").asNumber() == 99);
}

// Verifies deserialize() on malformed JSON reports PARSE_ERROR rather than
// crashing.
//
// NOTE: unlike Record::deserialize, Page::deserialize does not wrap
// Json::parse in a try/catch. This test pins the intended contract (same
// as Record's) and is expected to surface as a thrown-exception FAIL under
// RUN's catch block until Page::deserialize gets the same try/catch guard
// Record::deserialize already has -- see Page.cpp Section 4.
static void deserialize_malformed_returns_parse_error() {
    Page p;
    CHK(p.deserialize("{not valid json") == Status::PARSE_ERROR);
}

// Verifies move construction transfers state and leaves the source in the
// documented moved-from state (invalid id, not dirty).
static void move_constructor_transfers_state() {
    Page original(5);
    CHK(original.addRecord(Record(1)) == Status::OK);

    Page moved(std::move(original));

    CHK(moved.getID() == 5);
    CHK(moved.recordCount() == 1);
    CHK(moved.isDirty());

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    CHK(original.getID() == DBConstants::INVALID_PAGE_ID);
    CHK(!original.isDirty());
}

// Verifies move assignment transfers state and leaves the source in the
// documented moved-from state.
static void move_assignment_transfers_state() {
    Page original(6);
    CHK(original.addRecord(Record(1)) == Status::OK);

    Page target(1);
    target = std::move(original);

    CHK(target.getID() == 6);
    CHK(target.recordCount() == 1);

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    CHK(original.getID() == DBConstants::INVALID_PAGE_ID);
    CHK(!original.isDirty());
}

// Executes all Page test cases.
static void run_tests() {
    RUN(default_constructor_has_invalid_id);
    RUN(explicit_constructor_sets_id_and_empty_state);
    RUN(add_record_increases_count_and_marks_dirty);
    RUN(add_record_fills_page_then_rejects);
    RUN(get_record_returns_matching_record);
    RUN(get_record_missing_returns_nullptr);
    RUN(get_record_skips_deleted);
    RUN(get_record_at_by_index);
    RUN(get_record_at_includes_deleted);
    RUN(update_record_existing_replaces_data);
    RUN(update_record_missing_returns_not_found);
    RUN(update_record_deleted_returns_not_found);
    RUN(delete_record_marks_deleted_and_dirty);
    RUN(delete_record_missing_returns_not_found);
    RUN(compact_removes_deleted_records);
    RUN(compact_frees_slots_for_reuse);
    RUN(to_json_excludes_deleted_records);
    RUN(json_round_trip_preserves_live_records);
    RUN(serialize_round_trip);
    RUN(deserialize_malformed_returns_parse_error);
    RUN(move_constructor_transfers_state);
    RUN(move_assignment_transfers_state);
}

REGISTER_TEST_SUITE();