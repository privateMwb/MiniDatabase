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

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Verifies the default constructor leaves the page with an invalid id and
// empty, clean state.
TEST(Page, DefaultConstructorHasInvalidId) {
    Page p;
    EXPECT_EQ(p.getID(), DBConstants::INVALID_PAGE_ID);
    EXPECT_TRUE(p.isEmpty());
    EXPECT_FALSE(p.isDirty());
}

// Verifies the explicit-id constructor sets id and starts empty/clean with
// a full complement of free slots.
TEST(Page, ExplicitConstructorSetsIdAndEmptyState) {
    Page p(42);
    EXPECT_EQ(p.getID(), 42);
    EXPECT_TRUE(p.isEmpty());
    EXPECT_EQ(p.recordCount(), 0);
    EXPECT_EQ(p.freeSlots(), DBConstants::MAX_RECORDS_PAGE);
    EXPECT_FALSE(p.isDirty());
}

// Verifies addRecord increases the record count and marks the page dirty.
TEST(Page, AddRecordIncreasesCountAndMarksDirty) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(10)), Status::OK);

    EXPECT_EQ(p.recordCount(), 1);
    EXPECT_FALSE(p.isEmpty());
    EXPECT_TRUE(p.isDirty());
}

// Verifies that once a page holds MAX_RECORDS_PAGE records it reports full
// and rejects further inserts with OUT_OF_MEMORY.
TEST(Page, AddRecordFillsPageThenRejects) {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        ASSERT_EQ(p.addRecord(Record(i)), Status::OK);
    }

    EXPECT_TRUE(p.isFull());
    EXPECT_EQ(p.freeSlots(), 0);
    EXPECT_EQ(p.addRecord(Record(9999)), Status::OUT_OF_MEMORY);
}

// Verifies getRecord finds a record by id.
TEST(Page, GetRecordReturnsMatchingRecord) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(5)), Status::OK);

    Record* got = p.getRecord(5);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->getID(), 5);
}

// Verifies getRecord on a missing id returns nullptr rather than crashing.
TEST(Page, GetRecordMissingReturnsNullptr) {
    Page p(1);
    EXPECT_EQ(p.getRecord(123), nullptr);
}

// Verifies getRecord treats a soft-deleted record as not found.
TEST(Page, GetRecordSkipsDeleted) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(1)), Status::OK);
    ASSERT_EQ(p.deleteRecord(1), Status::OK);

    EXPECT_EQ(p.getRecord(1), nullptr);
}

// Verifies getRecordAt returns records by position and nullptr past the end.
TEST(Page, GetRecordAtByIndex) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(7)), Status::OK);
    ASSERT_EQ(p.addRecord(Record(8)), Status::OK);

    Record* first = p.getRecordAt(0);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->getID(), 7);

    Record* second = p.getRecordAt(1);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getID(), 8);

    EXPECT_EQ(p.getRecordAt(2), nullptr);
}

// Verifies getRecordAt, unlike getRecord(id), does NOT filter out
// soft-deleted records -- it's a raw positional accessor. Callers that need
// live-only records (e.g. QueryEngine's scan loop) must check isDeleted().
TEST(Page, GetRecordAtIncludesDeleted) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(1)), Status::OK);
    ASSERT_EQ(p.deleteRecord(1), Status::OK);

    Record* r = p.getRecordAt(0);
    ASSERT_NE(r, nullptr);
    EXPECT_TRUE(r->isDeleted());
}

// Verifies updateRecord replaces the stored data for an existing record.
TEST(Page, UpdateRecordExistingReplacesData) {
    Page p(1);
    Record original(1);
    ASSERT_EQ(original.setField("x", Json(1)), Status::OK);
    ASSERT_EQ(p.addRecord(original), Status::OK);

    Record updated(1);
    ASSERT_EQ(updated.setField("x", Json(2)), Status::OK);
    ASSERT_EQ(p.updateRecord(updated), Status::OK);

    Record* got = p.getRecord(1);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->getField("x").asNumber(), 2);
}

// Verifies updateRecord on a missing id reports NOT_FOUND.
TEST(Page, UpdateRecordMissingReturnsNotFound) {
    Page p(1);
    EXPECT_EQ(p.updateRecord(Record(555)), Status::NOT_FOUND);
}

// Verifies updateRecord treats a soft-deleted record as not found (it goes
// through the same lookup as getRecord).
TEST(Page, UpdateRecordDeletedReturnsNotFound) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(1)), Status::OK);
    ASSERT_EQ(p.deleteRecord(1), Status::OK);

    EXPECT_EQ(p.updateRecord(Record(1)), Status::NOT_FOUND);
}

// Verifies deleteRecord soft-deletes: the record becomes unreachable via
// getRecord but still occupies a slot (recordCount unchanged) until
// compact() runs.
TEST(Page, DeleteRecordMarksDeletedAndDirty) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(1)), Status::OK);
    ASSERT_EQ(p.deleteRecord(1), Status::OK);

    EXPECT_EQ(p.getRecord(1), nullptr);
    EXPECT_TRUE(p.isDirty());
    EXPECT_EQ(p.recordCount(), 1);
}

// Verifies deleteRecord on a missing id reports NOT_FOUND.
TEST(Page, DeleteRecordMissingReturnsNotFound) {
    Page p(1);
    EXPECT_EQ(p.deleteRecord(999), Status::NOT_FOUND);
}

// Verifies compact() drops soft-deleted records, keeps live ones, and
// clears the dirty flag.
TEST(Page, CompactRemovesDeletedRecords) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(1)), Status::OK);
    ASSERT_EQ(p.addRecord(Record(2)), Status::OK);
    ASSERT_EQ(p.deleteRecord(1), Status::OK);

    ASSERT_EQ(p.compact(), Status::OK);

    EXPECT_EQ(p.recordCount(), 1);
    EXPECT_FALSE(p.isDirty());
    EXPECT_NE(p.getRecord(2), nullptr);
    EXPECT_EQ(p.getRecord(1), nullptr);
}

// Verifies compact() reclaims slots occupied by soft-deleted records, so a
// full page becomes not-full again after compacting.
TEST(Page, CompactFreesSlotsForReuse) {
    Page p(1);
    for (RecordID i = 0; i < DBConstants::MAX_RECORDS_PAGE; ++i) {
        ASSERT_EQ(p.addRecord(Record(i)), Status::OK);
    }
    ASSERT_TRUE(p.isFull());

    ASSERT_EQ(p.deleteRecord(0), Status::OK);
    ASSERT_TRUE(p.isFull()); // soft-deleted slot not reclaimed yet

    ASSERT_EQ(p.compact(), Status::OK);
    EXPECT_FALSE(p.isFull());
    EXPECT_EQ(p.freeSlots(), 1);
}

// Verifies toJson excludes soft-deleted records and preserves the page id.
TEST(Page, ToJsonExcludesDeletedRecords) {
    Page p(1);
    ASSERT_EQ(p.addRecord(Record(10)), Status::OK);
    ASSERT_EQ(p.addRecord(Record(20)), Status::OK);
    ASSERT_EQ(p.deleteRecord(20), Status::OK);

    Json envelope = p.toJson();
    ASSERT_EQ(envelope["__page_id__"].asNumber(), 1);

    const Json::ArrayType& arr = envelope["records"].asArray();
    ASSERT_EQ(arr.size(), 1);

    bool foundTen = false;
    for (const Json& entry : arr) {
        if (static_cast<RecordID>(entry["__id__"].asNumber()) == 10)
            foundTen = true;
    }
    EXPECT_TRUE(foundTen);
}

// Verifies toJson()/fromJson() preserves id and live record data. The
// soft-deleted record is intentionally excluded from toJson, so it will
// not reappear after the round trip -- that's expected, not a bug.
TEST(Page, JsonRoundTripPreservesLiveRecords) {
    Page original(4);
    Record r1(1);
    ASSERT_EQ(r1.setField("name", Json("Ada")), Status::OK);
    Record r2(2);
    ASSERT_EQ(r2.setField("name", Json("Grace")), Status::OK);

    ASSERT_EQ(original.addRecord(r1), Status::OK);
    ASSERT_EQ(original.addRecord(r2), Status::OK);
    ASSERT_EQ(original.deleteRecord(2), Status::OK);

    Json envelope = original.toJson();

    Page restored;
    ASSERT_EQ(restored.fromJson(envelope), Status::OK);

    EXPECT_EQ(restored.getID(), 4);
    EXPECT_EQ(restored.recordCount(), 1);
    EXPECT_FALSE(restored.isDirty());

    Record* got = restored.getRecord(1);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->getField("name").asString(), "Ada");
}

// Verifies serialize()/deserialize() round trip matches toJson()/fromJson().
TEST(Page, SerializeRoundTrip) {
    Page original(2);
    Record r(1);
    ASSERT_EQ(r.setField("v", Json(99)), Status::OK);
    ASSERT_EQ(original.addRecord(r), Status::OK);

    std::string raw = original.serialize();

    Page restored;
    ASSERT_EQ(restored.deserialize(raw), Status::OK);

    EXPECT_EQ(restored.getID(), 2);
    EXPECT_EQ(restored.recordCount(), 1);

    Record* got = restored.getRecord(1);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->getField("v").asNumber(), 99);
}

// Verifies deserialize() on malformed JSON reports PARSE_ERROR rather than
// crashing.
//
// NOTE: unlike Record::deserialize, Page::deserialize does not wrap
// Json::parse in a try/catch. This test pins the intended contract (same
// as Record's) and is expected to surface as a thrown-exception failure
// until Page::deserialize gets the same try/catch guard Record::deserialize
// already has -- see Page.cpp Section 4.
TEST(Page, DeserializeMalformedReturnsParseError) {
    Page p;
    EXPECT_EQ(p.deserialize("{not valid json"), Status::PARSE_ERROR);
}

// Verifies move construction transfers state and leaves the source in the
// documented moved-from state (invalid id, not dirty).
TEST(Page, MoveConstructorTransfersState) {
    Page original(5);
    ASSERT_EQ(original.addRecord(Record(1)), Status::OK);

    Page moved(std::move(original));

    EXPECT_EQ(moved.getID(), 5);
    EXPECT_EQ(moved.recordCount(), 1);
    EXPECT_TRUE(moved.isDirty());

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_EQ(original.getID(), DBConstants::INVALID_PAGE_ID);
    EXPECT_FALSE(original.isDirty());
}

// Verifies move assignment transfers state and leaves the source in the
// documented moved-from state.
TEST(Page, MoveAssignmentTransfersState) {
    Page original(6);
    ASSERT_EQ(original.addRecord(Record(1)), Status::OK);

    Page target(1);
    target = std::move(original);

    EXPECT_EQ(target.getID(), 6);
    EXPECT_EQ(target.recordCount(), 1);

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_EQ(original.getID(), DBConstants::INVALID_PAGE_ID);
    EXPECT_FALSE(original.isDirty());
}
