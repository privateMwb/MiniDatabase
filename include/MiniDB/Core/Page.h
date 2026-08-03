/**
 * @file            Page.h
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
#include <MiniDB/Core/Record.h>

// clang-format off
#include <VectorPro/Vector.h> // VectorPro::Vector (record pointers)
#include <PoolPro/Pool.h>     // PoolPro::Pool (fixed-capacity Record storage)
// clang-format on

// A fixed-capacity storage unit holding up to DBConstants::MAX_RECORDS_PAGE
// records. Records are pool-allocated (not individually heap-allocated) so
// a full page's worth of records lives in one contiguous allocation, and
// deletion is soft (Record::markDeleted()) so a page's record count/order
// stays stable until an explicit compact() reclaims deleted slots.

namespace MiniDB::Core {

using namespace MiniDB::Common;
using namespace VectorPro;
using namespace PoolPro;

/**
 * @brief A fixed-capacity page of records, the unit of storage StorageEngine
 * reads/writes to disk.
 * @details Records are allocated from a `Pool<>` sized for
 * `DBConstants::MAX_RECORDS_PAGE` `Record`s and referenced via `records`,
 * a `Vector<Record*>` preserving insertion order. Deletion
 * (`deleteRecord()`) is soft — the record stays in `records` and still
 * occupies pool memory (and still counts toward `isFull()`) until
 * `compact()` reclaims it.
 */
class Page {
  public:
    PageID id;          ///< Unique identifier for this page.
    bool dirty = false; ///< `true` if this page has unsaved changes since the last save/compact.

  private:
    Vector<Record*> records; ///< Pool-allocated records, in insertion order.
    Pool<> pool;             ///< Backing storage for up to `DBConstants::MAX_RECORDS_PAGE` records.

  public:
    /// @brief Constructs an empty page with `id == DBConstants::INVALID_PAGE_ID`.
    Page();

    /// @brief Constructs an empty page with the given `id`.
    /// @param id Identifier to assign to this page.
    explicit Page(PageID id);

    /// @brief Destroys every record still held by this page and releases the pool.
    ~Page();

    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;

    /**
     * @brief Move-constructs a page, taking ownership of `other`'s
     * records and pool.
     * @param other Page to move from. Left with
     * `id == DBConstants::INVALID_PAGE_ID` and `dirty == false`.
     */
    Page(Page&& other) noexcept;

    /**
     * @brief Move-assigns from `other`, destroying this page's existing
     * records first.
     * @param other Page to move from. Left with
     * `id == DBConstants::INVALID_PAGE_ID` and `dirty == false`.
     * @return Reference to `*this`.
     */
    Page& operator=(Page&& other) noexcept;

    /**
     * @brief Copies `record` into this page's pool and appends it.
     * @param record Record to copy in.
     * @return `Status::OK` on success; `Status::OUT_OF_MEMORY` if the
     * page is already full or the pool allocation fails.
     */
    [[nodiscard]] Status addRecord(const Record& record);

    /**
     * @brief Finds the (non-deleted) record with the given id.
     * @param id Record id to search for.
     * @return Pointer to the record, or `nullptr` if not present or
     * soft-deleted.
     */
    [[nodiscard]] Record* getRecord(RecordID id);

    /// @copydoc getRecord(RecordID)
    [[nodiscard]] const Record* getRecord(RecordID id) const;

    /**
     * @brief Returns the record at storage slot `index`, regardless of
     * its deleted state.
     * @param index Zero-based slot index, in insertion order.
     * @return Pointer to the record, or `nullptr` if `index` is out of
     * range.
     */
    [[nodiscard]] Record* getRecordAt(std::size_t index);

    /// @copydoc getRecordAt(std::size_t)
    [[nodiscard]] const Record* getRecordAt(std::size_t index) const;

    /**
     * @brief Replaces the field data of the record matching
     * `record.getID()` with `record.data`.
     * @param record Record supplying the id to match and the new data.
     * @return `Status::OK` on success, `Status::NOT_FOUND` if no
     * matching (non-deleted) record exists.
     */
    [[nodiscard]] Status updateRecord(const Record& record);

    /**
     * @brief Soft-deletes the record with the given id.
     * @param id Record id to delete.
     * @return `Status::OK` on success, `Status::NOT_FOUND` if no
     * matching (non-deleted) record exists.
     * @details The record stays in storage (still occupies pool memory
     * and still counts toward `isFull()`) until `compact()` reclaims it.
     */
    [[nodiscard]] Status deleteRecord(RecordID id);

    /**
     * @brief Physically removes all soft-deleted records, reclaiming
     * their pool slots.
     * @return `Status::OK`, always.
     * @details Clears `dirty` on completion.
     */
    [[nodiscard]] Status compact();

    /**
     * @brief Builds a `Json` envelope containing this page's id and its
     * non-deleted records.
     * @return A `Json` object with `"__page_id__"` and `"records"` keys.
     * @details Builds the tree directly (via each `Record::toJson()`)
     * with no intermediate dump()/parse() round trip per record; prefer
     * this (and `fromJson()`) over `serialize()`/`deserialize()` except
     * at the outermost file-I/O boundary.
     */
    [[nodiscard]] Json toJson() const;

    /**
     * @brief Populates this page's id and records from a `Json` envelope
     * produced by `toJson()`.
     * @param envelope Envelope to parse.
     * @return `Status::OK` on success; `Status::PARSE_ERROR` if
     * `envelope` is null; `Status::OUT_OF_MEMORY` if a record's pool
     * allocation fails; or any `Status` returned by a record's
     * `fromJson()` on malformed record data.
     * @details Clears `dirty` on success.
     */
    [[nodiscard]] Status fromJson(const Json& envelope);

    /// @brief Returns this page serialized to a JSON string (`toJson().dump()`).
    [[nodiscard]] std::string serialize() const;

    /**
     * @brief Parses `raw` as JSON and populates this page from it.
     * @param raw JSON text produced by `serialize()`.
     * @return `Status::OK` on success, `Status::PARSE_ERROR` if `raw`
     * isn't valid JSON or parses to a null envelope.
     */
    [[nodiscard]] Status deserialize(const std::string& raw);

    /// @brief Returns whether this page holds `DBConstants::MAX_RECORDS_PAGE` records.
    [[nodiscard]] bool isFull() const noexcept;
    /// @brief Returns whether this page has unsaved changes.
    [[nodiscard]] bool isDirty() const noexcept;
    /// @brief Returns whether this page holds no records.
    [[nodiscard]] bool isEmpty() const noexcept;
    /// @brief Returns the number of records currently stored, including soft-deleted ones.
    [[nodiscard]] std::size_t recordCount() const noexcept;
    /// @brief Returns the number of additional records this page can hold before it's full.
    [[nodiscard]] std::size_t freeSlots() const noexcept;
    /// @brief Returns this page's id.
    [[nodiscard]] PageID getID() const noexcept;
};

} // namespace MiniDB::Core
