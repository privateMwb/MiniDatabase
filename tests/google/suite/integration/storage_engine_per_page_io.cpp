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

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

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
    Status s = p.addRecord(r);
    EXPECT_EQ(s, Status::OK); // surface as a failed check rather than silently continuing
    return p;
}

} // namespace

// Verifies multiple pages of the same table round-trip correctly by id.
TEST(StorageEnginePerPageIO, MultiplePagesSameTableRoundTrip) {
    StorageEngine engine(tempDir("multi_page"));

    for (PageID id = 0; id < 5; ++id) {
        Page p = makePage(id, id, "page_" + std::to_string(id));
        ASSERT_EQ(engine.writePage("users", p), Status::OK);
    }

    for (PageID id = 0; id < 5; ++id) {
        Page out;
        ASSERT_EQ(engine.readPageFromDisk("users", id, out), Status::OK);
        EXPECT_EQ(out.getID(), id);
        const Record* r = out.getRecord(id);
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->getField("name").asString(), "page_" + std::to_string(id));
    }
}

// Verifies two different tables sharing a data directory don't clobber
// each other's page 0 (or any other page id).
TEST(StorageEnginePerPageIO, TwoTablesDoNotClobberEachOther) {
    StorageEngine engine(tempDir("two_tables"));

    Page usersPage = makePage(0, 1, "Ada");
    Page ordersPage = makePage(0, 1, "widget");

    ASSERT_EQ(engine.writePage("users", usersPage), Status::OK);
    ASSERT_EQ(engine.writePage("orders", ordersPage), Status::OK);

    Page outUsers, outOrders;
    ASSERT_EQ(engine.readPageFromDisk("users", 0, outUsers), Status::OK);
    ASSERT_EQ(engine.readPageFromDisk("orders", 0, outOrders), Status::OK);

    EXPECT_EQ(outUsers.getRecord(1)->getField("name").asString(), "Ada");
    EXPECT_EQ(outOrders.getRecord(1)->getField("name").asString(), "widget");
}

// Verifies a fresh StorageEngine instance (simulating a process restart --
// empty cache, same data directory) can still fetch pages a previous
// instance wrote to disk.
TEST(StorageEnginePerPageIO, ColdStartReadsPagesFromPreviousInstance) {
    std::string dir = tempDir("cold_start");

    {
        StorageEngine writer(dir);
        for (PageID id = 0; id < 3; ++id) {
            Page p = makePage(id, id, "user_" + std::to_string(id));
            ASSERT_EQ(writer.writePage("users", p), Status::OK);
        }
    } // `writer` (and its cache) goes out of scope here

    StorageEngine reader(dir);
    for (PageID id = 0; id < 3; ++id) {
        EXPECT_FALSE(reader.isCached("users", id)); // nothing cached yet
        Page* p = reader.fetchPage("users", id);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(p->getRecord(id)->getField("name").asString(), "user_" + std::to_string(id));
        EXPECT_TRUE(reader.isCached("users", id)); // now cached after the fetch
    }
}

// Verifies a page adopted into the cache via cachePage() (never written to
// disk directly) is correctly persisted once evicted, and readable back
// via a completely independent read path.
TEST(StorageEnginePerPageIO, CachePageThenEvictPersistsToDisk) {
    std::string dir = tempDir("cachepage_evict");
    StorageEngine engine(dir);

    Page p = makePage(7, 1, "Grace");
    ASSERT_TRUE(p.isDirty()); // addRecord marks it dirty
    engine.cachePage("users", std::move(p));

    ASSERT_EQ(engine.evictPage("users", 7), Status::OK);
    EXPECT_FALSE(engine.isCached("users", 7));

    Page out;
    ASSERT_EQ(engine.readPageFromDisk("users", 7, out), Status::OK);
    EXPECT_EQ(out.getRecord(1)->getField("name").asString(), "Grace");
}

// Verifies readPageFromDisk for a page id that was never written returns
// NOT_FOUND, distinguishing "no such page yet" from an actual error, even
// once other pages exist in the same file.
TEST(StorageEnginePerPageIO, UnwrittenPageIdReturnsNotFoundAlongsideExistingOnes) {
    StorageEngine engine(tempDir("sparse"));

    Page p = makePage(10, 1, "only page ten");
    ASSERT_EQ(engine.writePage("users", p), Status::OK);

    Page out;
    EXPECT_EQ(engine.readPageFromDisk("users", 3, out), Status::NOT_FOUND);
    EXPECT_EQ(engine.readPageFromDisk("users", 10, out), Status::OK);
}
