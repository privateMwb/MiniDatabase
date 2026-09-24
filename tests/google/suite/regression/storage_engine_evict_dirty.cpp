// StorageEngine Evict-Dirty-Page Regression Test
// Pins the fix where evictPage() used to erase the cache entry
// unconditionally, silently discarding unwritten changes to a dirty page.
// Fixed to flush before erasing, and to surface write failures instead of
// dropping data.
//
// Covers:
// - evicting a dirty page flushes it to disk before removing it from cache
// - evicting an already-clean page still succeeds and removes it (the
//   flush-before-evict path must be a no-op for clean pages, not a skip)
// - evicting a page that was never cached is an idempotent no-op

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

namespace {
std::string tempDir(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path() / ("minidb_regression_" + label);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}
} // namespace

// Verifies evicting a DIRTY cached page flushes it to disk first: the
// record it held is readable from disk after eviction, not lost.
TEST(StorageEngineEvictDirty, EvictDirtyPageFlushesFirst) {
    StorageEngine engine(tempDir("evict_dirty"));

    Page page(3);
    ASSERT_EQ(page.addRecord(Record(1)), Status::OK);
    ASSERT_TRUE(page.isDirty());

    engine.cachePage("orders", std::move(page));
    ASSERT_TRUE(engine.isCached("orders", 3));

    ASSERT_EQ(engine.evictPage("orders", 3), Status::OK);
    EXPECT_FALSE(engine.isCached("orders", 3));

    Page reloaded;
    ASSERT_EQ(engine.readPageFromDisk("orders", 3, reloaded), Status::OK);
    EXPECT_EQ(reloaded.recordCount(), 1);
    EXPECT_NE(reloaded.getRecord(1), nullptr);
}

// Verifies evicting a CLEAN cached page (already flushed) still succeeds
// and removes it from the cache -- the flush-before-evict path must be a
// no-op for clean pages, not skip eviction entirely.
TEST(StorageEngineEvictDirty, EvictCleanPageRemoves) {
    StorageEngine engine(tempDir("evict_clean"));

    Page page(4);
    ASSERT_EQ(page.addRecord(Record(1)), Status::OK);
    engine.cachePage("orders", std::move(page));
    ASSERT_EQ(engine.flushPage("orders", 4), Status::OK); // now clean

    ASSERT_EQ(engine.evictPage("orders", 4), Status::OK);
    EXPECT_FALSE(engine.isCached("orders", 4));

    Page reloaded;
    ASSERT_EQ(engine.readPageFromDisk("orders", 4, reloaded), Status::OK);
    EXPECT_EQ(reloaded.recordCount(), 1);
}

// Verifies evicting a page that was never cached is an idempotent no-op
// (Status::OK), matching the documented "wasn't cached: idempotent no-op"
// semantics -- not an error, and not a crash.
TEST(StorageEngineEvictDirty, EvictUncachedPageNoop) {
    StorageEngine engine(tempDir("evict_missing"));

    ASSERT_FALSE(engine.isCached("orders", 7));
    EXPECT_EQ(engine.evictPage("orders", 7), Status::OK);
}
