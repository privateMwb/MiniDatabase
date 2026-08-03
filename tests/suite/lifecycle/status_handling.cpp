// Contract: Status Reporting, Never Silent Data Loss
// Every fallible operation in this codebase reports failure via a
// [[nodiscard]] Status return rather than throwing across its own API
// boundary or silently discarding data. This file checks that contract at
// two of its most important points:
//   - malformed input to a deserialize()/fromJson() path returns
//     Status::PARSE_ERROR rather than letting an exception escape
//   - StorageEngine never drops a dirty cached page without either
//     persisting it or reporting a failure Status
//
// Covers: Record, Page, Table deserialize(); StorageEngine
// evictPage/flushPage/writePage/fetchPage.

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

namespace {

Vector<ColumnDef> makeSchema() {
    return Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}};
}

std::string tempDir(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path() / ("minidb_status_test_" + label);
    std::filesystem::create_directories(dir);
    return dir.string();
}

} // namespace

// --- No exceptions escape a deserialize boundary on malformed input -------

static void record_deserialize_malformed_returns_parse_error() {
    Record r;
    CHK(r.deserialize("not json at all") == Status::PARSE_ERROR);
}

static void page_deserialize_malformed_returns_parse_error() {
    Page p;
    CHK(p.deserialize("not json at all") == Status::PARSE_ERROR);
}

static void table_deserialize_malformed_returns_parse_error() {
    Table t("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
    CHK(t.deserialize("not json at all") == Status::PARSE_ERROR);
}

// --- StorageEngine never silently drops a dirty page -----------------------

// Verifies that evicting a dirty cached page writes it to disk first, and
// that the write is actually durable (readable back via a fresh read that
// bypasses the cache).
static void evict_dirty_page_persists_before_removal() {
    StorageEngine engine(tempDir("evict_dirty"));

    Page p(0);
    Record r(1);
    (void)r.setField("name", Json("Ada"));
    CHK(p.addRecord(r) == Status::OK);
    CHK(p.isDirty() == true);

    engine.cachePage("users", std::move(p));
    CHK(engine.isCached("users", 0) == true);

    CHK(engine.evictPage("users", 0) == Status::OK);
    CHK(engine.isCached("users", 0) == false);

    // Bypass the cache entirely: read directly from disk to confirm the
    // eviction actually persisted the data rather than discarding it.
    Page fromDisk;
    CHK(engine.readPageFromDisk("users", 0, fromDisk) == Status::OK);
    CHK(fromDisk.recordCount() == 1);

    const Record* out = fromDisk.getRecord(1);
    CHK(out != nullptr);
    CHK(out->getField("name").asString() == "Ada");
}

// Verifies evicting a page that was never cached is an idempotent no-op
// (Status::OK), not an error.
static void evict_uncached_page_is_idempotent_ok() {
    StorageEngine engine(tempDir("evict_uncached"));
    CHK(engine.evictPage("users", 42) == Status::OK);
}

// Verifies flushPage on an uncached page returns NOT_FOUND (distinct from
// evictPage's idempotent-OK semantics -- flushPage reports precisely
// whether there was anything to flush; evictPage folds NOT_FOUND into OK
// itself since "nothing to evict" is a normal outcome for eviction).
static void flush_uncached_page_returns_not_found() {
    StorageEngine engine(tempDir("flush_uncached"));
    CHK(engine.flushPage("users", 42) == Status::NOT_FOUND);
}

// Verifies flushPage on a clean (non-dirty) cached page is a no-op that
// still reports Status::OK.
static void flush_clean_page_is_ok_noop() {
    StorageEngine engine(tempDir("flush_clean"));

    Page p(0); // never modified: not dirty
    CHK(p.isDirty() == false);
    engine.cachePage("users", std::move(p));

    CHK(engine.flushPage("users", 0) == Status::OK);
}

// Verifies writePage rejects a page whose serialized form doesn't fit the
// fixed PAGE_SIZE slot budget with Status::OUT_OF_MEMORY, rather than
// silently truncating it on disk.
static void writepage_oversized_returns_out_of_memory() {
    StorageEngine engine(tempDir("oversized"));

    Page p(0);
    // Force the serialized page past PAGE_SIZE (4096 bytes) with a handful
    // of large string fields -- each record's JSON overhead plus a long
    // string comfortably exceeds the budget well before MAX_RECORDS_PAGE.
    std::string bigValue(2000, 'x');
    for (RecordID i = 0; i < 5; ++i) {
        Record r(i);
        (void)r.setField("blob", Json(bigValue));
        CHK(p.addRecord(r) == Status::OK);
    }

    CHK(engine.writePage("users", p) == Status::OUT_OF_MEMORY);
}

// Verifies fetchPage on a fresh StorageEngine instance (empty cache) loads
// a previously-written page from disk rather than reporting a miss as
// permanent.
static void fetchpage_loads_from_disk_on_cache_miss() {
    std::string dir = tempDir("fetch_miss");

    {
        StorageEngine writer(dir);
        Page p(0);
        Record r(1);
        (void)r.setField("name", Json("Ada"));
        CHK(p.addRecord(r) == Status::OK);
        CHK(writer.writePage("users", p) == Status::OK);
    }

    StorageEngine reader(dir);
    CHK(reader.isCached("users", 0) == false); // fresh instance, empty cache

    Page* fetched = reader.fetchPage("users", 0);
    CHK(fetched != nullptr);
    CHK(fetched->recordCount() == 1);
}

// Executes all status/error-handling contract checks.
static void run_tests() {
    RUN(record_deserialize_malformed_returns_parse_error);
    RUN(page_deserialize_malformed_returns_parse_error);
    RUN(table_deserialize_malformed_returns_parse_error);
    RUN(evict_dirty_page_persists_before_removal);
    RUN(evict_uncached_page_is_idempotent_ok);
    RUN(flush_uncached_page_returns_not_found);
    RUN(flush_clean_page_is_ok_noop);
    RUN(writepage_oversized_returns_out_of_memory);
    RUN(fetchpage_loads_from_disk_on_cache_miss);
}

REGISTER_TEST_SUITE();
