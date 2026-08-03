// EvictPage() Is Not A Delete.
//
// Demonstrates:
// - evictPage() only removes a page from StorageEngine's in-memory cache
//   -- it does not delete anything from disk
// - a dirty page is flushed to disk BEFORE eviction, so the data survives
//   the eviction; it's still fully readable via readPageFromDisk/fetchPage
//   afterward, just no longer cached
// - the actual "delete" a person might be picturing -- removing a record
//   -- happens at the Table level (deleteRecord), an entirely different,
//   higher-level operation

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

static void run_examples() {
    std::string dir = (std::filesystem::temp_directory_path() / "minidb_example_evict").string();
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    StorageEngine engine(dir);

    // Put a dirty page (one with unsaved changes) into the cache.
    setTitle("Cache A Dirty Page");

    Page page(1);
    Record ada(1);
    (void)ada.setField("name", Json("Ada"));
    (void)page.addRecord(ada);

    std::cout << "page.isDirty() before caching : " << page.isDirty() << " (true)\n";
    engine.cachePage("users", std::move(page));
    std::cout << "isCached(\"users\", 1)          : " << engine.isCached("users", 1)
              << " (true)\n\n";

    // The misuse: it's tempting to assume evictPage() "removes" or
    // "deletes" the page's data, the way deleteRecord() removes a record.
    // It doesn't -- it's a cache-management operation, not a data-deletion
    // one. Before the page is dropped from the cache, it's flushed to disk
    // if it was dirty.

    setTitle("evictPage() Flushes, Then Removes From Cache Only");

    Status s = engine.evictPage("users", 1);
    std::cout << "evictPage(\"users\", 1) status : " << static_cast<int>(s) << " (OK)\n";
    std::cout << "isCached(\"users\", 1) after   : " << engine.isCached("users", 1)
              << " (false)\n\n";

    // The data is completely intact on disk -- evicting it from the cache
    // didn't touch the underlying record at all.
    setTitle("The Data Is Still There On Disk");

    Page fromDisk;
    s = engine.readPageFromDisk("users", 1, fromDisk);
    std::cout << "readPageFromDisk status : " << static_cast<int>(s) << " (OK)\n";
    std::cout << "recordCount             : " << fromDisk.recordCount()
              << " (1 -- nothing was deleted)\n";
    std::cout << "record's name           : " << fromDisk.getRecord(1)->getField("name").asString()
              << "\n\n";

    // fetchPage() proves the same thing from the cache's own perspective:
    // asking for the page again after eviction transparently reloads it
    // from disk -- it wouldn't be able to if evictPage() had deleted it.
    setTitle("fetchPage() After Eviction Reloads From Disk");

    Page* refetched = engine.fetchPage("users", 1);
    std::cout << "fetchPage() returned nullptr? : " << (refetched == nullptr)
              << " (false -- it's still there)\n";
    if (refetched) {
        std::cout << "refetched recordCount         : " << refetched->recordCount() << "\n";
    }

    // If you actually want to remove a record, that's a Table-level
    // operation entirely separate from anything StorageEngine's cache does.
    setTitle("What Actually Deletes: Table::deleteRecord()");
    std::cout << "(deleteRecord() marks a record deleted within a Table you already have\n";
    std::cout << " open in memory -- it has nothing to do with StorageEngine's page cache.)\n";

    std::filesystem::remove_all(dir);
}

REGISTER_EXAMPLE_SUITE();
