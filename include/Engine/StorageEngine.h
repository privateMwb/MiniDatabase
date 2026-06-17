#pragma once

#include <string>

#include "../Common/Type.h"
#include "../Core/Database.h"
#include "../Core/Table.h"
#include "../Core/Page.h"

#include "../../libs/LRUCache/LRUCache.h"

class StorageEngine {
    private:
    
    // State 
    std::string             dataDirectory;
    LRUCache<PageID, Page>  pageCache;
    
    public:
    
    // Constructor
    explicit StorageEngine(const std::string& dataDirectory);
    
    // Persistence
    [[nodiscard]] Status saveDatabase(const Database& db, const std::string& fileName) const;
    [[nodiscard]] Status loadDatabase(Database& db, const std::string& fileName) const;
    
    // Page Cache 
    [[nodiscard]] Page* getCachedPage(PageID id);
    void               cachePage(PageID id, Page page);
    [[nodiscard]] bool isCached(PageID id) const;
    void               evictPage(PageID id);
    
    // Cache Stats 
    [[nodiscard]] double       cacheHitRate()  const;
    [[nodiscard]] std::size_t  cacheHits()     const;
    [[nodiscard]] std::size_t  cacheMisses()   const;
    
};
