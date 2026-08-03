// Integration: StorageEngine Per-Page I/O
// Exercises writePage/readPageFromDisk/fetchPage/cachePage/evictPage
// together against real files, with multiple pages and multiple tables in
// the same data directory -- verifying table page files don't clobber each
// other, and that a "cold start" (fresh StorageEngine instance, empty
// cache, simulating a process restart) can still read back everything a
// previous instance wrote.
//
// Covers:
// - multiple pages within one table's page file, read back correctly by id
// - two different tables' page files coexist without clobbering each other
//   despite sharing a data directory
// - a fresh StorageEngine instance (empty cache) reads back pages written
//   by a previous instance ("cold start" / process-restart simulation)
// - cachePage -> evictPage persists to disk even when the page was never
//   written via writePage directly

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

namespace {

std::string tempDir(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path() / ("minidb_storageengine_" + label);
    std::filesystem::create_directories(dir);
    return dir.string();
}

Page makePage(PageID id, RecordID recordId, const std::string& name) {
    Page p(id);
    Record r(recordId);
    (void)r.setField("name", Json(name));
    if (Status s = p.addRecord(r); s != Status::OK) {
        CHK(s == Status::OK); // surface as a failed check rather than silently continuing
    }
    return p;
}

} // namespace

// Verifies multiple pages of the same table round-trip correctly by id.
static void multiple_pages_same_table_round_trip() {
    StorageEngine engine(tempDir("multi_page"));

    for (PageID id = 0; id < 5; ++id) {
        Page p = makePage(id, id, "page_" + std::to_string(id));
        CHK(engine.writePage("users", p) == Status::OK);
    }

    for (PageID id = 0; id < 5; ++id) {
        Page out;
        CHK(engine.readPageFromDisk("users", id, out) == Status::OK);
        CHK(out.getID() == id);
        const Record* r = out.getRecord(id);
        CHK(r != nullptr);
        CHK(r->getField("name").asString() == "page_" + std::to_string(id));
    }
}

// Verifies two different tables sharing a data directory don't clobber
// each other's page 0 (or any other page id).
static void two_tables_do_not_clobber_each_other() {
    StorageEngine engine(tempDir("two_tables"));

    Page usersPage = makePage(0, 1, "Ada");
    Page ordersPage = makePage(0, 1, "widget");

    CHK(engine.writePage("users", usersPage) == Status::OK);
    CHK(engine.writePage("orders", ordersPage) == Status::OK);

    Page outUsers, outOrders;
    CHK(engine.readPageFromDisk("users", 0, outUsers) == Status::OK);
    CHK(engine.readPageFromDisk("orders", 0, outOrders) == Status::OK);

    CHK(outUsers.getRecord(1)->getField("name").asString() == "Ada");
    CHK(outOrders.getRecord(1)->getField("name").asString() == "widget");
}

// Verifies a fresh StorageEngine instance (simulating a process restart --
// empty cache, same data directory) can still fetch pages a previous
// instance wrote to disk.
static void cold_start_reads_pages_from_previous_instance() {
    std::string dir = tempDir("cold_start");

    {
        StorageEngine writer(dir);
        for (PageID id = 0; id < 3; ++id) {
            Page p = makePage(id, id, "user_" + std::to_string(id));
            CHK(writer.writePage("users", p) == Status::OK);
        }
    } // `writer` (and its cache) goes out of scope here

    StorageEngine reader(dir);
    for (PageID id = 0; id < 3; ++id) {
        CHK(reader.isCached("users", id) == false); // nothing cached yet
        Page* p = reader.fetchPage("users", id);
        CHK(p != nullptr);
        CHK(p->getRecord(id)->getField("name").asString() == "user_" + std::to_string(id));
        CHK(reader.isCached("users", id) == true); // now cached after the fetch
    }
}

// Verifies a page adopted into the cache via cachePage() (never written to
// disk directly) is correctly persisted once evicted, and readable back
// via a completely independent read path.
static void cachepage_then_evict_persists_to_disk() {
    std::string dir = tempDir("cachepage_evict");
    StorageEngine engine(dir);

    Page p = makePage(7, 1, "Grace");
    CHK(p.isDirty() == true); // addRecord marks it dirty
    engine.cachePage("users", std::move(p));

    CHK(engine.evictPage("users", 7) == Status::OK);
    CHK(engine.isCached("users", 7) == false);

    Page out;
    CHK(engine.readPageFromDisk("users", 7, out) == Status::OK);
    CHK(out.getRecord(1)->getField("name").asString() == "Grace");
}

// Verifies readPageFromDisk for a page id that was never written returns
// NOT_FOUND, distinguishing "no such page yet" from an actual error, even
// once other pages exist in the same file.
static void unwritten_page_id_returns_not_found_alongside_existing_ones() {
    StorageEngine engine(tempDir("sparse"));

    Page p = makePage(10, 1, "only page ten");
    CHK(engine.writePage("users", p) == Status::OK);

    Page out;
    CHK(engine.readPageFromDisk("users", 3, out) == Status::NOT_FOUND);
    CHK(engine.readPageFromDisk("users", 10, out) == Status::OK);
}

// Executes all StorageEngine per-page I/O integration checks.
static void run_tests() {
    RUN(multiple_pages_same_table_round_trip);
    RUN(two_tables_do_not_clobber_each_other);
    RUN(cold_start_reads_pages_from_previous_instance);
    RUN(cachepage_then_evict_persists_to_disk);
    RUN(unwritten_page_id_returns_not_found_alongside_existing_ones);
}

REGISTER_TEST_SUITE();
