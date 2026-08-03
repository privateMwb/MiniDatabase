// Custom Buffer Pool Usage.
//
// Demonstrates:
// - manually managing a StorageEngine page cache lifecycle: cache, check
//   dirty/cached state, flush, then evict
// - why evictPage() flushing first means the disk is never out of sync
//   with a page you've just removed from memory

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

static void run_examples() {
    // Building your own page from scratch and adopting it into the cache
    // -- no file on disk exists for it yet.
    setTitle("Adopt A Page Into The Cache");

    const std::string dir = "custom_buffer_pool";
    std::filesystem::create_directories(dir);
    StorageEngine storage(dir);

    Page page(0);
    (void)page.addRecord(Record(1));
    std::cout << "page is dirty (unsaved changes) : " << std::boolalpha << page.isDirty() << "\n";

    storage.cachePage("orders", std::move(page)); // Page is move-only
    std::cout << "isCached : " << storage.isCached("orders", 0) << "\n\n";

    // getCachedPage() peeks without ever touching disk -- useful to check
    // "do I already have this in memory?" before deciding whether to fetch.
    setTitle("Peek The Cache Without Touching Disk");

    Page* cached = storage.getCachedPage("orders", 0);
    std::cout << "getCachedPage found it : " << (cached != nullptr) << "\n\n";

    // flushPage() writes a dirty page to disk but leaves it in the cache.
    // Calling it on a page that's already clean is a documented no-op.
    setTitle("Flush Explicitly, Then Flush Again (no-op)");

    Status s = storage.flushPage("orders", 0);
    std::cout << "flushPage status (first call)  : " << static_cast<int>(s) << "\n";
    s = storage.flushPage("orders", 0);
    std::cout << "flushPage status (already clean) : " << static_cast<int>(s) << "\n\n";

    // evictPage() flushes (if dirty) before removing the page from the
    // cache -- so by the time it's gone from memory, disk already has
    // whatever was in it. This is why "evict" is safe to call as a plain
    // memory-pressure operation, not something that risks losing writes.
    setTitle("Evict, Then Confirm The Data Survived On Disk");

    s = storage.evictPage("orders", 0);
    std::cout << "evictPage status : " << static_cast<int>(s) << "\n";
    std::cout << "isCached after evict : " << storage.isCached("orders", 0) << "\n";

    Page reread;
    s = storage.readPageFromDisk("orders", 0, reread);
    std::cout << "readPageFromDisk status : " << static_cast<int>(s) << "\n";
    std::cout << "record count survived : " << reread.recordCount() << "\n\n";

    // Cache hit/miss stats are tracked for you -- useful for judging
    // whether a cache is actually paying for itself in a real workload.
    setTitle("Cache Stats");

    std::cout << "cacheHits    : " << storage.cacheHits() << "\n";
    std::cout << "cacheMisses  : " << storage.cacheMisses() << "\n";
    std::cout << "cacheHitRate : " << storage.cacheHitRate() << "\n";

    std::filesystem::remove_all(dir);
}

REGISTER_EXAMPLE_SUITE();