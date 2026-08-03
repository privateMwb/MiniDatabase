/**
 * @file            Table.h
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
#include <MiniDB/Core/Page.h>
#include <MiniDB/Core/Record.h>

// clang-format off
#include <VectorPro/Vector.h>   // VectorPro::Vector (schema, pages)
#include <HashMapPro/HashMap.h> // HashMapPro::HashMap (record/page indices)
// clang-format on

// A named, schema-bound collection of records, split across fixed-capacity
// Pages. Two indices keep lookups O(1): `index` maps RecordID -> the PageID
// holding it, `pageIndex` maps PageID -> Page* (replacing what would
// otherwise be a linear scan over `pages`). Page ids come from a monotonic
// counter independent of pages.size(), so a page id is never reused even
// after page removal.

namespace MiniDB::Core {

using namespace MiniDB::Common;
using namespace VectorPro;
using namespace HashMapPro;

/**
 * @brief A named collection of records conforming to a fixed schema,
 * stored across one or more `Page`s.
 * @details Records are validated against `schema` on insert/update and
 * routed to storage via two O(1) indices: `index` (`RecordID` ->
 * `PageID`) and `pageIndex` (`PageID` -> `Page*`). New records are
 * placed in the first page with a free slot, or a newly allocated page
 * if none has room. Page ids are assigned from a monotonic counter
 * (`nextPageId_`), independent of `pages.size()`, so removing/compacting
 * pages can never cause an id to be reused.
 */
class Table {
  public:
    std::string name; ///< This table's name.
    TableID id;       ///< Unique identifier for this table within its database.

  private:
    Vector<ColumnDef> schema; ///< Column definitions every record is validated against.

    Vector<Page*> pages;              ///< All pages belonging to this table, in creation order.
    HashMap<RecordID, PageID> index;  ///< O(1) lookup: which page holds a given record.
    HashMap<PageID, Page*> pageIndex; ///< O(1) lookup: page id to page pointer.

    /// Monotonic page-id counter, independent of `pages.size()` (using
    /// size() as the id source caused id reuse/collision once page
    /// removal exists; this counter never goes backwards).
    PageID nextPageId_ = 0;

    bool dirty = false; ///< `true` if this table has unsaved changes.

  public:
    /**
     * @brief Constructs a table with the given name, id, and schema.
     * @param name Table name.
     * @param id Identifier to assign to this table.
     * @param schema Column definitions records will be validated against.
     */
    Table(std::string name, TableID id, Vector<ColumnDef> schema);

    /// @brief Destroys every page owned by this table.
    ~Table();

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;

    /**
     * @brief Move-constructs a table, taking ownership of `other`'s
     * pages, indices, and schema.
     * @param other Table to move from. Left with
     * `id == DBConstants::INVALID_TABLE_ID`, `dirty == false`, and a
     * reset page-id counter.
     */
    Table(Table&& other) noexcept;

    /**
     * @brief Move-assigns from `other`, destroying this table's existing
     * pages first.
     * @param other Table to move from. Left with
     * `id == DBConstants::INVALID_TABLE_ID`, `dirty == false`, and a
     * reset page-id counter.
     * @return Reference to `*this`.
     */
    Table& operator=(Table&& other) noexcept;

    /**
     * @brief Validates and inserts `record`.
     * @param record Record to insert.
     * @return `Status::OK` on success; whatever `Record::validate()`
     * returns if schema validation fails; `Status::DUPLICATE_KEY` if a
     * record with this id already exists; `Status::OUT_OF_MEMORY` if no
     * page has room and a new page can't be allocated, or the target
     * page rejects the insert.
     */
    [[nodiscard]] Status insertRecord(const Record& record);

    /**
     * @brief Looks up the record with the given id.
     * @param id Record id to search for.
     * @param out Destination record, overwritten with a copy on success.
     * @return `Status::OK` on success, `Status::NOT_FOUND` if no such
     * record exists (or its owning page can no longer be found).
     */
    [[nodiscard]] Status getRecord(RecordID id, Record& out) const;

    /**
     * @brief Validates and replaces the field data of the record
     * matching `record.getID()`.
     * @param record Record supplying the id to match and the new data.
     * @return `Status::OK` on success; whatever `Record::validate()`
     * returns if schema validation fails; `Status::NOT_FOUND` if no
     * matching record exists.
     */
    [[nodiscard]] Status updateRecord(const Record& record);

    /**
     * @brief Deletes the record with the given id.
     * @param id Record id to delete.
     * @return `Status::OK` on success, `Status::NOT_FOUND` if no such
     * record exists.
     * @details Deletion is soft at the `Page` level — see
     * `Page::deleteRecord()` — but this table's `index` entry is removed
     * immediately.
     */
    [[nodiscard]] Status deleteRecord(RecordID id);

    /// @brief Returns this table's column definitions.
    [[nodiscard]] const Vector<ColumnDef>& getSchema() const noexcept;

    /**
     * @brief Checks whether the schema declares a column named `name`.
     * @param name Column name to search for.
     * @return `true` if `name` is a declared column.
     */
    [[nodiscard]] bool hasColumn(const FieldName& name) const noexcept;

    /**
     * @brief Compacts every page, physically removing soft-deleted
     * records.
     * @return `Status::OK` on success, or the first non-OK status
     * returned by an individual page's `compact()`.
     * @details Clears `dirty` on success.
     */
    [[nodiscard]] Status compact();

    /// @brief Returns all pages belonging to this table.
    [[nodiscard]] const Vector<Page*>& getPages() const noexcept;

    /**
     * @brief Rebuilds `index` and `pageIndex` from scratch by scanning
     * every page's records.
     * @return `Status::OK`, always.
     * @details Used after loading pages directly (e.g. `fromJson()`)
     * rather than through `insertRecord()`, where the indices wouldn't
     * otherwise be populated.
     */
    [[nodiscard]] Status rebuildIndex();

    /**
     * @brief Builds a `Json` envelope containing this table's id, name,
     * schema, and pages.
     * @return A `Json` object with `"__table_id___"`, `"__name__"`,
     * `"schema"`, and `"page"` keys.
     * @details Builds the tree directly (via each `Page::toJson()`) with
     * no intermediate dump()/parse() round trip per page; prefer this
     * (and `fromJson()`) over `serialize()`/`deserialize()` except at
     * the outermost file-I/O boundary.
     */
    [[nodiscard]] Json toJson() const;

    /**
     * @brief Populates this table's id, name, schema, and pages from a
     * `Json` envelope produced by `toJson()`, then rebuilds its indices.
     * @param envelope Envelope to parse.
     * @return `Status::OK` on success; `Status::PARSE_ERROR` if
     * `envelope` is null; or any `Status` returned by a page's
     * `fromJson()` on malformed page data.
     * @details Resumes the monotonic page-id counter above the highest
     * loaded page id, so newly inserted pages after a load can never
     * collide with a loaded page id.
     */
    [[nodiscard]] Status fromJson(const Json& envelope);

    /// @brief Returns this table serialized to a JSON string (`toJson().dump()`).
    [[nodiscard]] std::string serialize() const;

    /**
     * @brief Parses `raw` as JSON and populates this table from it.
     * @param raw JSON text produced by `serialize()`.
     * @return `Status::OK` on success, `Status::PARSE_ERROR` if `raw`
     * isn't valid JSON or parses to a null envelope.
     */
    [[nodiscard]] Status deserialize(const std::string& raw);

    /// @brief Returns this table's name.
    [[nodiscard]] std::string getName() const noexcept;
    /// @brief Returns this table's id.
    [[nodiscard]] TableID getID() const noexcept;
    /// @brief Returns the total number of records across all pages.
    [[nodiscard]] std::size_t recordCount() const noexcept;
    /// @brief Returns the number of pages belonging to this table.
    [[nodiscard]] std::size_t pageCount() const noexcept;
    /// @brief Returns whether this table has unsaved changes.
    [[nodiscard]] bool isDirty() const noexcept;
    /// @brief Returns whether this table has no pages.
    [[nodiscard]] bool isEmpty() const noexcept;

  private:
    /**
     * @brief Finds the first page with a free slot, allocating a new
     * page if none exists.
     * @return Pointer to a page with room for at least one more record.
     * Never `nullptr` unless allocation of a new page fails.
     */
    [[nodiscard]] Page* findPageWithSlot() noexcept;

    /**
     * @brief Looks up a page by id via `pageIndex` in O(1).
     * @param id Page id to search for.
     * @return Pointer to the page, or `nullptr` if not present.
     */
    [[nodiscard]] Page* findPageByID(PageID id) noexcept;

    /// @copydoc findPageByID(PageID)
    [[nodiscard]] const Page* findPageByID(PageID id) const noexcept;

    /// @brief Returns the next page id and advances the monotonic counter.
    [[nodiscard]] PageID nextPageID() noexcept;
};

} // namespace MiniDB::Core
