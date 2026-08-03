/**
 * @file            Record.h
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

// clang-format off
#include <JsonPro/Json.h>     // JsonPro::Json (record field storage)
#include <VectorPro/Vector.h> // VectorPro::Vector (schema in validate())
// clang-format on

// A single row of data within a Table. Fields are stored untyped in a
// JsonPro::Json object rather than a fixed C++ struct, so a Record's
// shape can vary per row and evolve without a schema migration; `validate()`
// is what enforces a Table's schema against that otherwise-untyped storage
// at the boundaries that need it (insert, update, deserialize).

namespace MiniDB::Core {

using namespace MiniDB::Common;
using namespace JsonPro;
using namespace VectorPro;

/**
 * @brief A single row of data within a table, addressed by `RecordID`
 * and holding an arbitrary set of named fields.
 * @details Fields live in `data`, a `Json` object keyed by field name —
 * there's no fixed per-column storage, so `validate()` is the only place
 * a `Record`'s contents are checked against a `Table`'s schema.
 * Deletion is soft (`deleted` flag) rather than immediate removal, so a
 * `Table` can reclaim/compact deleted records in bulk instead of on
 * every delete.
 */
class Record {
  public:
    RecordID id;          ///< Unique identifier for this record within its table.
    Json data;            ///< Field storage, keyed by field name.
    bool deleted = false; ///< Soft-delete flag; `true` once `markDeleted()` has been called.

    /// @brief Constructs a record with `id == DBConstants::INVALID_RECORD_ID` and empty `data`.
    Record();

    /// @brief Constructs a record with the given `id` and empty `data`.
    /// @param id Identifier to assign to this record.
    explicit Record(RecordID id);

    /**
     * @brief Constructs a record with the given `id` and `data`.
     * @param id Identifier to assign to this record.
     * @param data Field data, moved in.
     */
    Record(RecordID id, Json data);

    /**
     * @brief Sets field `key` to `value`, creating the field if it
     * doesn't already exist.
     * @param key Field name. Must be non-empty.
     * @param value Value to store.
     * @return `Status::OK` on success, `Status::INVALID_SCHEMA` if `key`
     * is empty.
     */
    [[nodiscard]] Status setField(const FieldName& key, const Json& value);

    /**
     * @brief Returns a copy of field `key`'s value.
     * @param key Field name to look up.
     * @return The field's value, or a null `Json` if `key` isn't present.
     */
    [[nodiscard]] Json getField(const FieldName& key) const;

    /**
     * @brief Non-copying accessor for hot paths (predicate evaluation,
     * sort comparators).
     * @param key Field name to look up.
     * @return A reference into `data`, valid only as long as this
     * `Record` is not mutated or destroyed. If `key` is missing, returns
     * a reference to a shared static null `Json` rather than allocating
     * a fresh one.
     */
    [[nodiscard]] const Json& getFieldRef(const FieldName& key) const;

    /**
     * @brief Checks whether field `key` is present.
     * @param key Field name to check.
     * @return `true` if `key` exists in `data`.
     */
    [[nodiscard]] bool hasField(const FieldName& key) const;

    /**
     * @brief Removes field `key`, if present.
     * @param key Field name to remove.
     * @return `Status::OK` if removed, `Status::NOT_FOUND` if `key`
     * wasn't present.
     */
    [[nodiscard]] Status removeField(const FieldName& key);

    /**
     * @brief Validates every field in `data` against `schema`.
     * @param schema Column definitions to validate against.
     * @return `Status::OK` if every non-nullable column is present and
     * every present column's value matches its declared `ColumnType`;
     * `Status::INVALID_SCHEMA` if a required (non-nullable) column is
     * missing; `Status::INVALID_TYPE` if a present column's value
     * doesn't match its declared type.
     * @details Extra fields not listed in `schema` are not checked —
     * this only validates that `schema`'s requirements are satisfied,
     * not that `data` contains nothing else.
     */
    [[nodiscard]] Status validate(const Vector<ColumnDef>& schema) const;

    /**
     * @brief Builds a `Json` envelope containing this record's id,
     * deleted flag, and field data.
     * @return A `Json` object with `"__id__"`, `"__deleted__"`, and
     * `"data"` keys.
     * @details Builds the tree directly with no intermediate
     * dump()/parse() round trip; prefer this (and `fromJson()`) over
     * `serialize()`/`deserialize()` except at the outermost file-I/O
     * boundary.
     */
    [[nodiscard]] Json toJson() const;

    /**
     * @brief Populates this record's id, deleted flag, and field data
     * from a `Json` envelope produced by `toJson()`.
     * @param envelope Envelope to parse.
     * @return `Status::OK` on success, `Status::PARSE_ERROR` if
     * `envelope` is null.
     */
    [[nodiscard]] Status fromJson(const Json& envelope);

    /// @brief Returns this record serialized to a JSON string (`toJson().dump()`).
    [[nodiscard]] std::string serialize() const;

    /**
     * @brief Parses `raw` as JSON and populates this record from it.
     * @param raw JSON text produced by `serialize()`.
     * @return `Status::OK` on success, `Status::PARSE_ERROR` if `raw`
     * isn't valid JSON or parses to a null envelope.
     */
    [[nodiscard]] Status deserialize(const std::string& raw);

    /// @brief Returns whether this record has been soft-deleted.
    [[nodiscard]] bool isDeleted() const;

    /// @brief Marks this record as deleted. Does not remove it from storage.
    void markDeleted();

    /// @brief Returns this record's id.
    [[nodiscard]] RecordID getID() const;

    /// @brief Compares records by id only — field data is not considered.
    [[nodiscard]] bool operator==(const Record& other) const;
    /// @brief Compares records by id only — field data is not considered.
    [[nodiscard]] bool operator!=(const Record& other) const;
};

} // namespace MiniDB::Core
