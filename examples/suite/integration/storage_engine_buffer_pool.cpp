// StorageEngine Buffer Pool.
//
// Demonstrates:
// - using StorageEngine's per-page API directly (writePage/fetchPage/
//   evictPage) against real files, independent of Table
// - a Page can be persisted and cached without a Table ever owning it

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static void run_examples() {
    // StorageEngine only knows about Page and raw bytes on disk -- it has
    // no idea what a Table or a schema is. Each table's pages live in
    // "<dataDirectory>/<tableName>.pages", one fixed-size slot per page id.
    setTitle("Create A Standalone Page");

    const std::string dir = "storage_buffer_pool";
    std::filesystem::create_directories(dir);
    StorageEngine storage(dir);

    Page page(0);
    (void)page.addRecord(Record(1));
    (void)page.addRecord(Record(2));
    std::cout << "page record count : " << page.recordCount() << "\n\n";

    // writePage() persists the page directly -- there's no Table involved
    // at all, just a table *name* used to pick the file.
    setTitle("Write It To Disk");

    Status s = storage.writePage("orders", page);
    std::cout << "writePage status : " << static_cast<int>(s) << "\n\n";

    // readPageFromDisk() reads it straight back, bypassing the cache
    // entirely -- useful for verifying what's actually on disk.
    setTitle("Read It Back, Bypassing The Cache");

    Page fromDisk;
    s = storage.readPageFromDisk("orders", 0, fromDisk);
    std::cout << "readPageFromDisk status : " << static_cast<int>(s) << "\n";
    std::cout << "record count : " << fromDisk.recordCount() << "\n\n";

    // fetchPage() is the normal path: a cache miss transparently falls
    // through to readPageFromDisk() and adopts the result into the cache.
    setTitle("Fetch Through The Cache (miss, then hit)");

    std::cout << "isCached before fetch : " << std::boolalpha << storage.isCached("orders", 0)
              << "\n";

    Page* fetched = storage.fetchPage("orders", 0);
    std::cout << "fetchPage found a page : " << (fetched != nullptr) << "\n";
    std::cout << "isCached after fetch  : " << storage.isCached("orders", 0) << "\n";

    std::filesystem::remove_all(dir);
}

REGISTER_EXAMPLE_SUITE();