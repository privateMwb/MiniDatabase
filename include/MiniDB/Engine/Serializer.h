/**
 * @file            Serializer.h
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
#include <MiniDB/Core/Record.h>
#include <MiniDB/Core/Table.h>

#include <JsonPro/Json.h>

// A plain-array export/import format, distinct from Table/Database's own
// toJson()/fromJson() envelope format (see Table.h, Database.h): a table
// exports as a flat JSON array of its records' field data with an "id" key
// merged in, rather than the id/schema/pages envelope Table::toJson() uses.
// Meant for interchange with tools/consumers that just want "the records",
// not MiniDB's own internal structure.

namespace MiniDB::Engine {

using namespace MiniDB::Common;
using namespace MiniDB::Core;
using namespace JsonPro;

/**
 * @brief Stateless export/import utilities converting `Table`s and
 * `Database`s to/from a flat, array-of-records JSON format.
 * @details All members are static; this class exists purely for
 * namespacing. The export format differs from `Table::toJson()`'s own
 * envelope: each record's field data is exported directly, with an
 * `"id"` key merged in, rather than nested inside a schema/pages
 * envelope — see individual method docs for the exact shape.
 */
class Serializer {
  public:
    /**
     * @brief Exports every non-deleted record in `table` as a flat JSON
     * array.
     * @param table Table to export.
     * @return A JSON array string; each element is the record's field
     * data (`Record::data`) with an `"id"` key added holding the
     * record's `RecordID`.
     */
    [[nodiscard]] static std::string exportTableToJson(const Table& table);

    /**
     * @brief Imports records from a JSON array (as produced by
     * `exportTableToJson()`) into `table`.
     * @param table Table to insert the imported records into.
     * @param json JSON array text.
     * @return `Status::OK` on success; `Status::PARSE_ERROR` if `json`
     * doesn't parse or doesn't parse to an array; or any `Status`
     * returned by `Table::insertRecord()` on a rejected record.
     */
    [[nodiscard]] static Status importTableFromJson(Table& table, const std::string& json);

    /**
     * @brief Imports records from an already-parsed JSON array into
     * `table`.
     * @param table Table to insert the imported records into.
     * @param parsed Parsed JSON array, in the same shape
     * `exportTableToJson()` produces.
     * @return `Status::OK` on success; `Status::PARSE_ERROR` if `parsed`
     * is null or not an array; or any `Status` returned by
     * `Table::insertRecord()` on a rejected record.
     * @details Overload taking an already-parsed `Json` tree, so callers
     * that already hold a parsed subtree (e.g.
     * `importDatabaseFromJson()`) can skip a redundant dump()+parse()
     * round trip.
     * @details Each entry's `"id"` key, if present and numeric, is used
     * as the record's `RecordID` and stripped from the stored field data
     * (it's export metadata, not a real field); entries without an
     * `"id"` are assigned sequential ids starting from 0.
     */
    [[nodiscard]] static Status importTableFromJson(Table& table, const Json& parsed);

    /**
     * @brief Exports `table` to `path` as a flat JSON array (see
     * `exportTableToJson()`), written atomically.
     * @param table Table to export.
     * @param path Destination file path.
     * @return `Status::OK` on success, `Status::IO_ERROR` on write
     * failure.
     */
    [[nodiscard]] static Status exportTableToFile(const Table& table, const std::string& path);

    /**
     * @brief Imports `table` from a flat JSON array file (see
     * `importTableFromJson()`).
     * @param table Table to insert the imported records into.
     * @param path Source file path.
     * @return `Status::OK` on success; whatever `Status`
     * `FileIO::readFile()` returns on read failure; or any `Status`
     * returned by `importTableFromJson()` on malformed data.
     */
    [[nodiscard]] static Status importTableFromFile(Table& table, const std::string& path);

    /**
     * @brief Exports every table in `db` to a single JSON file, keyed by
     * table name.
     * @param db Database to export.
     * @param path Destination file path.
     * @return `Status::OK` on success, `Status::IO_ERROR` on write
     * failure.
     * @details Produces a JSON object whose keys are table names and
     * whose values are each table's flat record array, in the same
     * per-record shape `exportTableToJson()` uses. Written atomically.
     */
    [[nodiscard]] static Status exportDatabaseToJson(const Database& db, const std::string& path);

    /**
     * @brief Imports records from a file produced by
     * `exportDatabaseToJson()` into `db`'s existing tables.
     * @param db Database whose tables should receive the imported
     * records. Tables must already exist (e.g. via `createTable()`) —
     * this does not create tables.
     * @param path Source file path.
     * @return `Status::OK` on success; whatever `Status`
     * `FileIO::readFile()` returns on read failure; `Status::PARSE_ERROR`
     * if the file isn't valid JSON or parses to a null envelope; or any
     * `Status` returned by `importTableFromJson()` on malformed data.
     * @details Any table name in the file that doesn't already exist in
     * `db` is silently skipped rather than treated as an error.
     */
    [[nodiscard]] static Status importDatabaseFromJson(Database& db, const std::string& path);
};

} // namespace MiniDB::Engine
