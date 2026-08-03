/**
 * @file StorageEngine.cpp
 * @brief MiniDB::Engine::StorageEngine implementation.
 *
 * Contains the implementation of whole-database persistence and the
 * per-page buffer-pool (fixed-slot disk I/O backed by an LRU cache).
 */

// ============================================================
// Implementation for MiniDB::Engine::StorageEngine.
// ============================================================
//
//  Sections:
//   1. Constructor
//   2. Whole-Database Persistence
//   3. Helpers
//   4. Per-Page Disk I/O
//   5. Page Cache
//   6. Cache Stats
//
// ============================================================

#include <MiniDB/Engine/StorageEngine.h>
#include <MiniDB/Common/FileIO.h>

#include <cstdint>

namespace MiniDB::Engine {

// -----------------------------------------------------------------------
// Design note
//
// saveDatabase/loadDatabase are the primary durability path: a full,
// atomic, all-or-nothing dump/restore of the whole in-memory Database tree
// (Database::save/load). They are unrelated to the page cache below.
//
// writePage / readPageFromDisk / fetchPage / flushPage / evictPage are a
// separate, self-contained buffer-pool-style facility: real fixed-slot
// per-page disk I/O (see Common/FileIO.h's writeSlot/readSlot), wired to
// an actual LRU cache. They operate on independent Page copies -- Page is
// move-only, so the cache cannot alias a Page that another component (e.g.
// Table) still owns directly.
//
// Table does NOT yet route its reads/writes through this facility: it
// still holds its own fully-materialized Page* objects in memory and
// persists via the whole-tree path above. Migrating Table to fetch pages
// on demand through this cache -- so that it, rather than a full JSON
// dump, becomes the primary persistence mechanism -- is a further step and
// a materially larger change (it changes Table's core ownership model,
// not just StorageEngine's). What's here fixes the two concrete problems
// that existed previously: the cache was never backed by real disk I/O at
// all, and evicting a dirty page silently discarded it.
// -----------------------------------------------------------------------

// ============================================================
//  Section 1 — Constructor
// ============================================================
StorageEngine::StorageEngine(const std::string& dataDirectory)
    : dataDirectory(dataDirectory)
    , pageCache(DBConstants::LRU_CACHE_CAP) {}


// ============================================================
//  Section 2 — Whole-Database Persistence
// ============================================================
Status StorageEngine::saveDatabase(const Database& db, const std::string& filename) const {
    std::string fullpath = dataDirectory + "/" + filename;
    return db.save(fullpath);
}

Status StorageEngine::loadDatabase(Database& db, const std::string& filename) const {
    std::string fullpath = dataDirectory + "/" + filename;
    return db.load(fullpath);
}


// ============================================================
//  Section 3 — Helpers
// ============================================================
std::string StorageEngine::pageFilePath(const std::string& tableName) const {
    return dataDirectory + "/" + tableName + ".pages";
}

std::string StorageEngine::cacheKey(const std::string& tableName, PageID id) {
    return tableName + "#" + std::to_string(id);
}


// ============================================================
//  Section 4 — Per-Page Disk I/O
// ============================================================
Status StorageEngine::writePage(const std::string& tableName, const Page& page) const {
    return Common::FileIO::writeSlot(
        pageFilePath(tableName),
        static_cast<std::uint64_t>(page.getID()),
        DBConstants::PAGE_SIZE,
        page.serialize());
}

Status StorageEngine::readPageFromDisk(const std::string& tableName, PageID id, Page& out) const {
    std::string payload;
    Status s = Common::FileIO::readSlot(
        pageFilePath(tableName),
        static_cast<std::uint64_t>(id),
        DBConstants::PAGE_SIZE,
        payload);
    if (s != Status::OK) return s;   // NOT_FOUND: no such page written yet
    return out.deserialize(payload);
}


// ============================================================
//  Section 5 — Page Cache
// ============================================================
Page* StorageEngine::fetchPage(const std::string& tableName, PageID id) {
    std::string key = cacheKey(tableName, id);

    // `contains` first so a genuine cache hit is the only thing that
    // reaches `get` (and thus the only thing counted as a hit by the
    // underlying LRUCache's stats).
    if (pageCache.contains(key)) {
        return pageCache.get(key);
    }

    Page loaded(id);
    if (Status s = readPageFromDisk(tableName, id, loaded); s != Status::OK) {
        return nullptr;   // no such page on disk (or a read/parse failure)
    }

    pageCache.put(key, std::move(loaded));
    // NOTE: this access to the just-inserted entry may or may not itself be
    // counted as an additional "hit" by CachePro's LRUCache, depending on
    // its internal accounting -- unverified against that header. Flagging
    // as an assumption rather than asserting certainty either way.
    return pageCache.get(key);
}

Page* StorageEngine::getCachedPage(const std::string& tableName, PageID id) {
    return pageCache.get(cacheKey(tableName, id));
}

void StorageEngine::cachePage(const std::string& tableName, Page page) {
    std::string key = cacheKey(tableName, page.getID());
    pageCache.put(key, std::move(page));
}

bool StorageEngine::isCached(const std::string& tableName, PageID id) const {
    return pageCache.contains(cacheKey(tableName, id));
}

Status StorageEngine::flushPage(const std::string& tableName, PageID id) {
    std::string key = cacheKey(tableName, id);
    Page* cached = pageCache.get(key);
    if (!cached) return Status::NOT_FOUND;
    if (!cached->dirty) return Status::OK;   // clean: nothing to write

    Status s = writePage(tableName, *cached);
    if (s != Status::OK) return s;

    cached->dirty = false;   // `dirty` is a public data member on Page.
    return Status::OK;
}

Status StorageEngine::evictPage(const std::string& tableName, PageID id) {
    Status s = flushPage(tableName, id);
    if (s == Status::NOT_FOUND) return Status::OK;   // wasn't cached: idempotent no-op
    if (s != Status::OK) return s;                   // real write failure: do NOT drop the page

    (void)pageCache.erase(cacheKey(tableName, id));
    return Status::OK;
}


// ============================================================
//  Section 6 — Cache Stats
// ============================================================
double       StorageEngine::cacheHitRate()  const { return pageCache.hitRate(); }
std::size_t  StorageEngine::cacheHits()     const { return pageCache.hitCount(); }
std::size_t  StorageEngine::cacheMisses()   const { return pageCache.missCount(); }

} // namespace MiniDB::Engine
