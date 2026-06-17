#include "../../include/Engine/StorageEngine.h"

// Constructor
StorageEngine::StorageEngine(const std::string& dataDirectory) 
: dataDirectory(dataDirectory) 
, pageCache(DBConstants::LRU_CACHE_CAP) {}

// Persistence
Status StorageEngine::saveDatabase(const Database& db, const std::string& filename) const {
    std::string fullpath = dataDirectory + "/" + filename;
    return db.save(fullpath);
}

Status StorageEngine::loadDatabase(Database& db, const std::string& filename) const {
    std::string fullpath = dataDirectory + "/" + filename;
    return db.load(fullpath);
}

// Page Cache 
Page* StorageEngine::getCachedPage(PageID id) {
    return pageCache.get(id);
}

void StorageEngine::cachePage(PageID id, Page page) {
    pageCache.put(id, std::move(page));
}

bool StorageEngine::isCached(PageID id) const {
    return pageCache.contains(id);
}

void StorageEngine::evictPage(PageID id) {
    (void)pageCache.erase(id);
}

// Cache Stats 
double       StorageEngine::cacheHitRate()  const { return pageCache.hitRate(); }
std::size_t  StorageEngine::cacheHits()     const { return pageCache.hitCount(); }
std::size_t  StorageEngine::cacheMisses()   const { return pageCache.missCount(); }








