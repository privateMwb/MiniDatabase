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

#include <support/framework.h>

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
static void evict_dirty_page_flushes_first() {
    StorageEngine engine(tempDir("evict_dirty"));

    Page page(3);
    CHK(page.addRecord(Record(1)) == Status::OK);
    CHK(page.isDirty());

    engine.cachePage("orders", std::move(page));
    CHK(engine.isCached("orders", 3));

    CHK(engine.evictPage("orders", 3) == Status::OK);
    CHK(!engine.isCached("orders", 3));

    Page reloaded;
    CHK(engine.readPageFromDisk("orders", 3, reloaded) == Status::OK);
    CHK(reloaded.recordCount() == 1);
    CHK(reloaded.getRecord(1) != nullptr);
}

// Verifies evicting a CLEAN cached page (already flushed) still succeeds
// and removes it from the cache -- the flush-before-evict path must be a
// no-op for clean pages, not skip eviction entirely.
static void evict_clean_page_removes() {
    StorageEngine engine(tempDir("evict_clean"));

    Page page(4);
    CHK(page.addRecord(Record(1)) == Status::OK);
    engine.cachePage("orders", std::move(page));
    CHK(engine.flushPage("orders", 4) == Status::OK); // now clean

    CHK(engine.evictPage("orders", 4) == Status::OK);
    CHK(!engine.isCached("orders", 4));

    Page reloaded;
    CHK(engine.readPageFromDisk("orders", 4, reloaded) == Status::OK);
    CHK(reloaded.recordCount() == 1);
}

// Verifies evicting a page that was never cached is an idempotent no-op
// (Status::OK), matching the documented "wasn't cached: idempotent no-op"
// semantics -- not an error, and not a crash.
static void evict_uncached_page_noop() {
    StorageEngine engine(tempDir("evict_missing"));

    CHK(!engine.isCached("orders", 7));
    CHK(engine.evictPage("orders", 7) == Status::OK);
}

static void run_tests() {
    RUN(evict_dirty_page_flushes_first);
    RUN(evict_clean_page_removes);
    RUN(evict_uncached_page_noop);
}

REGISTER_TEST_SUITE();