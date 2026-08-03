/**
 * @file            Database.h
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
#include <MiniDB/Core/Table.h>

// clang-format off
#include <VectorPro/Vector.h>   // VectorPro::Vector (table list)
#include <HashMapPro/HashMap.h> // HashMapPro::HashMap (table indices)
// clang-format on

// The top-level entry point: a named collection of Tables, persisted as a
// single JSON file. Two indices keep lookups O(1): `index` maps table name
// -> TableID, `tableIndex` maps TableID -> Table*. Table ids come from a
// monotonic counter independent of tables.size(), so an id is never reused
// even after dropTable() shrinks the table list.

namespace MiniDB::Core {

using namespace MiniDB::Common;
using namespace VectorPro;
using namespace HashMapPro;

/**
 * @brief The top-level database object: a named collection of `Table`s,
 * loadable from and savable to a single file.
 * @details Tables are looked up by name via two O(1) indices: `index`
 * (table name -> `TableID`) and `tableIndex` (`TableID` -> `Table*`).
 * Table ids are assigned from a monotonic counter (`nextTableId_`),
 * independent of `tables.size()`, so dropping a table can never cause a
 * later `createTable()` to reuse an id still held by a surviving table.
 * `save()`/`load()` persist the whole database as one JSON document,
 * written atomically so a crash or I/O failure mid-write can never
 * corrupt the existing on-disk file.
 */
class Database {
  public:
    std::string name; ///< This database's name.

  private:
    Vector<Table*> tables;               ///< All tables in this database, in creation order.
    HashMap<std::string, TableID> index; ///< O(1) lookup: table name to id.
    HashMap<TableID, Table*> tableIndex; ///< O(1) lookup: table id to table pointer.

    /// Monotonic table-id counter, independent of `tables.size()` (which
    /// shrinks on `dropTable()` and previously caused id reuse/collisions).
    TableID nextTableId_ = 0;

    bool dirty = false; ///< `true` if this database has unsaved changes.

  public:
    /// @brief Constructs an empty database with the given name.
    /// @param name Database name.
    explicit Database(std::string name);

    /// @brief Destroys every table owned by this database.
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * @brief Move-constructs a database, taking ownership of `other`'s
     * tables and indices.
     * @param other Database to move from. Left empty and reusable.
     */
    Database(Database&& other) noexcept;

    /**
     * @brief Move-assigns from `other`, destroying this database's
     * existing tables first.
     * @param other Database to move from. Left empty and reusable.
     * @return Reference to `*this`.
     */
    Database& operator=(Database&& other) noexcept;

    /**
     * @brief Creates a new, empty table.
     * @param name Table name. Must not already exist in this database.
     * @param schema Column definitions for the new table.
     * @return `Status::OK` on success; `Status::OUT_OF_MEMORY` if the
     * database already holds `DBConstants::MAX_TABLES` tables;
     * `Status::TABLE_ALREADY_EXISTS` if `name` is already in use.
     */
    [[nodiscard]] Status createTable(const std::string& name, Vector<ColumnDef> schema);

    /**
     * @brief Deletes the table named `name`.
     * @param name Table name to drop.
     * @return `Status::OK` on success, `Status::TABLE_NOT_FOUND` if no
     * such table exists.
     */
    [[nodiscard]] Status dropTable(const std::string& name);

    /**
     * @brief Looks up a table by name.
     * @param name Table name to search for.
     * @return Pointer to the table, or `nullptr` if not present.
     */
    [[nodiscard]] Table* getTable(const std::string& name);

    /// @copydoc getTable(const std::string&)
    [[nodiscard]] const Table* getTable(const std::string& name) const;

    /**
     * @brief Checks whether a table named `name` exists.
     * @param name Table name to check.
     * @return `true` if present.
     */
    [[nodiscard]] bool hasTable(const std::string& name) const noexcept;

    /**
     * @brief Saves this database to `path` as a single JSON document,
     * written atomically.
     * @param path Destination file path.
     * @return `Status::OK` on success, `Status::IO_ERROR` on write
     * failure.
     * @details Builds the full document with one `toJson()`/`dump()`
     * (no intermediate per-table/per-page/per-record round trips) and
     * writes it via `FileIO::writeFileAtomic()`, so a crash or I/O
     * failure mid-write never corrupts the existing on-disk file.
     */
    [[nodiscard]] Status save(const std::string& path) const;

    /**
     * @brief Loads this database from a JSON document previously written
     * by `save()`.
     * @param path Source file path.
     * @return `Status::OK` on success; whatever `Status` `FileIO::readFile()`
     * returns on read failure; `Status::PARSE_ERROR` if the file isn't
     * valid JSON or parses to a null envelope; or any `Status` returned
     * by `fromJson()` on malformed data.
     */
    [[nodiscard]] Status load(const std::string& path);

    /**
     * @brief Compacts every table, physically removing soft-deleted
     * records.
     * @return `Status::OK` on success, or the first non-OK status
     * returned by an individual table's `compact()`.
     * @details Clears `dirty` on success.
     */
    [[nodiscard]] Status compact();

    /**
     * @brief Builds a `Json` envelope containing this database's name
     * and all its tables.
     * @return A `Json` object with `"__db_name__"` and `"tables"` keys.
     * @details Builds the tree directly (via each `Table::toJson()`)
     * with no intermediate dump()/parse() round trip per table.
     */
    [[nodiscard]] Json toJson() const;

    /**
     * @brief Populates this database's name and tables from a `Json`
     * envelope produced by `toJson()`.
     * @param envelope Envelope to parse.
     * @return `Status::OK` on success; `Status::PARSE_ERROR` if
     * `envelope` is null; `Status::OUT_OF_MEMORY` if the envelope
     * contains more than `DBConstants::MAX_TABLES` tables; or any
     * `Status` returned by a table's `fromJson()` on malformed table
     * data.
     * @details Parses every table into scratch containers first and
     * only replaces this database's live state once all of them succeed
     * — a failure partway through leaves the existing database
     * untouched rather than half-loaded. Resumes the monotonic table-id
     * counter above the highest loaded table id.
     */
    [[nodiscard]] Status fromJson(const Json& envelope);

    /// @brief Returns this database's name.
    [[nodiscard]] std::string getName() const noexcept;
    /// @brief Returns the number of tables in this database.
    [[nodiscard]] std::size_t tableCount() const noexcept;
    /// @brief Returns the total number of records across all tables.
    [[nodiscard]] std::size_t recordCount() const noexcept;
    /// @brief Returns whether this database has unsaved changes.
    [[nodiscard]] bool isDirty() const noexcept;
    /// @brief Returns whether this database has no tables.
    [[nodiscard]] bool isEmpty() const noexcept;
    /// @brief Returns all tables in this database.
    [[nodiscard]] const Vector<Table*>& getTables() const noexcept;

  private:
    /// @brief Returns the next table id and advances the monotonic counter.
    [[nodiscard]] TableID nextTableID() noexcept;
};

} // namespace MiniDB::Core

/// @brief Umbrella alias so this library's types are reachable as
/// `rain::Type`, alongside every other project library, while its true
/// namespace remains `MiniDB`. Reopens `rain` rather than aliasing it,
/// since multiple libraries each contribute their own names into the
/// same `rain` namespace -- an alias (`namespace rain = MiniDB;`) can
/// only ever bind to one target and collides the moment a second library
/// declares its own `rain` alias to something else. Declared here only
/// (MiniDB's main header); no other MiniDB header redeclares this.
namespace rain {
using namespace MiniDB;
}
