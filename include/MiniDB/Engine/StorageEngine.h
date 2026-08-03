/**
 * @file            StorageEngine.h
 *
 * @date            2026-2-8
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 MWB
 *                  All rights reserved.
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

#include <string>

#include <MiniDB/Common/Type.h>
#include <MiniDB/Core/Database.h>
#include <MiniDB/Core/Page.h>
#include <MiniDB/Core/Table.h>

#include <CachePro/LRUCache.h>

// Two independent persistence paths live here:
//
//   - saveDatabase()/loadDatabase(): the primary durability path, a full
//     atomic dump/restore of the whole in-memory Database tree
//     (Database::save/load). Unrelated to the page cache below.
//
//   - writePage()/readPageFromDisk()/fetchPage()/flushPage()/evictPage():
//     a separate, self-contained buffer-pool facility -- real fixed-slot
//     per-page disk I/O (Common/FileIO.h's writeSlot/readSlot) backed by
//     an LRU cache. Table does not yet route its reads/writes through
//     this; it still holds fully-materialized Page* objects and persists
//     via the whole-tree path above. These operate on independent Page
//     copies -- Page is move-only, so the cache can never alias a Page
//     another component (e.g. Table) still owns directly.

namespace MiniDB::Engine {

using namespace MiniDB::Common;
using namespace MiniDB::Core;
using namespace CachePro;

/**
 * @brief Disk persistence for MiniDB: whole-database save/load, plus an
 * independent per-page buffer-pool (fixed-slot disk I/O backed by an LRU
 * cache).
 * @details The two facilities this class provides are unrelated to each
 * other — see the file-level design note. `saveDatabase()`/
 * `loadDatabase()` remain the primary durability path; the page cache is
 * a self-contained facility not yet wired into `Table`'s own storage.
 */
class StorageEngine {
  private:
    std::string dataDirectory; ///< Root directory all files are read from/written to.

    /// Buffer-pool style page cache. Keyed by "<tableName>#<pageId>"
    /// rather than PageID alone, since a PageID is only unique within
    /// its own table. (A packed numeric key would be marginally cheaper
    /// per lookup than the string concatenation this costs -- but that
    /// would require StorageEngine to own its own table-name -> handle
    /// allocator, i.e. the same kind of ad hoc id-management that caused
    /// the Table/Database id-collision bugs fixed earlier. Not worth it
    /// unless profiling shows this lookup matters.)
    LRUCache<std::string, Page> pageCache;

    /// @brief Returns the on-disk path for `tableName`'s page file
    /// (`<dataDirectory>/<tableName>.pages`).
    [[nodiscard]] std::string pageFilePath(const std::string& tableName) const;

    /// @brief Builds the page-cache key for a given table and page id.
    [[nodiscard]] static std::string cacheKey(const std::string& tableName, PageID id);

  public:
    /// @brief Constructs a storage engine rooted at `dataDirectory`.
    /// @param dataDirectory Root directory for all reads/writes. Must
    /// already exist.
    explicit StorageEngine(const std::string& dataDirectory);

    /**
     * @brief Saves `db` to `<dataDirectory>/<fileName>` as a single
     * atomic JSON document.
     * @param db Database to save.
     * @param fileName Filename, relative to `dataDirectory`.
     * @return Whatever `Status` `Database::save()` returns.
     * @details Full logical dump of schema + all table/page/record data
     * in one atomic, all-or-nothing file. This remains the primary
     * durability path, independent of the per-page cache below.
     */
    [[nodiscard]] Status saveDatabase(const Database& db, const std::string& fileName) const;

    /**
     * @brief Loads `db` from `<dataDirectory>/<fileName>`, as written by
     * `saveDatabase()`.
     * @param db Database to load into.
     * @param fileName Filename, relative to `dataDirectory`.
     * @return Whatever `Status` `Database::load()` returns.
     */
    [[nodiscard]] Status loadDatabase(Database& db, const std::string& fileName) const;

    /**
     * @brief Writes `page` into its fixed slot in `tableName`'s page
     * file.
     * @param tableName Owning table's name; determines the target file.
     * @param page Page to write. Written at slot `page.getID()`, byte
     * range `[id * DBConstants::PAGE_SIZE, (id+1) * DBConstants::PAGE_SIZE)`.
     * @return `Status::OK` on success, `Status::OUT_OF_MEMORY` if the
     * page's serialized size doesn't fit `DBConstants::PAGE_SIZE` (never
     * silently truncated), `Status::IO_ERROR` on write failure.
     * @details Acts on the caller's own `Page` object and is independent
     * of the cache below — safe to call on any `Page` (e.g. one a
     * `Table` still owns directly) without adopting it into the cache.
     */
    [[nodiscard]] Status writePage(const std::string& tableName, const Page& page) const;

    /**
     * @brief Reads the page at slot `id` from `tableName`'s page file.
     * @param tableName Owning table's name; determines the source file.
     * @param id Page id / slot index to read.
     * @param out Destination page, overwritten on success.
     * @return `Status::OK` on success; `Status::NOT_FOUND` if no such
     * page has been written yet; `Status::PARSE_ERROR` if the slot's
     * stored data is corrupt.
     * @details Independent of the cache below — see `writePage()`.
     */
    [[nodiscard]] Status readPageFromDisk(const std::string& tableName, PageID id, Page& out) const;

    /**
     * @brief Returns the page for `(tableName, id)`, loading it from
     * disk into the cache on a miss.
     * @param tableName Owning table's name.
     * @param id Page id to fetch.
     * @return Pointer to the (now cached) page, owned by the cache and
     * valid until evicted; `nullptr` if no such page exists on disk
     * either.
     */
    [[nodiscard]] Page* fetchPage(const std::string& tableName, PageID id);

    /**
     * @brief Peeks the cache for `(tableName, id)` without touching disk.
     * @param tableName Owning table's name.
     * @param id Page id to look up.
     * @return Pointer to the cached page, or `nullptr` if not cached.
     */
    [[nodiscard]] Page* getCachedPage(const std::string& tableName, PageID id);

    /**
     * @brief Adopts `page` into the cache under `(tableName, page.getID())`.
     * @param tableName Owning table's name.
     * @param page Page to move into the cache. `Page` is move-only, so
     * this always transfers ownership rather than copying.
     */
    void cachePage(const std::string& tableName, Page page);

    /**
     * @brief Checks whether `(tableName, id)` is currently cached.
     * @param tableName Owning table's name.
     * @param id Page id to check.
     * @return `true` if cached.
     */
    [[nodiscard]] bool isCached(const std::string& tableName, PageID id) const;

    /**
     * @brief Writes the cached page for `(tableName, id)` to disk if
     * dirty.
     * @param tableName Owning table's name.
     * @param id Page id to flush.
     * @return `Status::OK` if clean (nothing to write) or the write
     * succeeded; `Status::NOT_FOUND` if not cached; otherwise whatever
     * `Status` `writePage()` returns on failure.
     */
    [[nodiscard]] Status flushPage(const std::string& tableName, PageID id);

    /**
     * @brief Flushes (see `flushPage()`) then removes the page for
     * `(tableName, id)` from the cache.
     * @param tableName Owning table's name.
     * @param id Page id to evict.
     * @return `Status::OK` if evicted, or if the page wasn't cached to
     * begin with (idempotent no-op); otherwise whatever `Status`
     * `flushPage()` returns on a real write failure — in which case the
     * page is *not* dropped from the cache, so unwritten changes are
     * never silently lost.
     */
    [[nodiscard]] Status evictPage(const std::string& tableName, PageID id);

    /// @brief Returns the page cache's hit rate as a percentage (0–100).
    [[nodiscard]] double cacheHitRate() const;
    /// @brief Returns the number of page-cache lookups that were hits.
    [[nodiscard]] std::size_t cacheHits() const;
    /// @brief Returns the number of page-cache lookups that were misses.
    [[nodiscard]] std::size_t cacheMisses() const;
};

} // namespace MiniDB::Engine
