/**
 * @file            Type.h
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

// clang-format off
#include <string>  // std::string (FieldName)
#include <cstdint> // uint32_t, uint64_t, UINT32_MAX, UINT64_MAX
// clang-format on

// Shared value types, enums, and constants used across every MiniDB
// module (Core, Engine, Common). Kept dependency-free (no MiniDB headers
// included) so it can sit at the bottom of the include graph without
// risking a cycle -- everything else includes this, this includes nothing
// of MiniDB's own.

namespace MiniDB::Common {

/// @brief Supported column value types for a table schema.
enum class ColumnType { INT, DOUBLE, STRING, BOOL };

/// @brief Comparison operators usable in query predicates.
enum class Op {
    EQ,  ///< Equal (`==`).
    NEQ, ///< Not equal (`!=`).
    GT,  ///< Greater than (`>`).
    GTE, ///< Greater than or equal (`>=`).
    LT,  ///< Less than (`<`).
    LTE  ///< Less than or equal (`<=`).
};

/// @brief Sort direction for query results.
enum class SortOrder { ASC, DESC };

/// @brief Outcome of a MiniDB operation, returned instead of throwing so
/// callers can handle expected failure modes (missing table, bad schema,
/// I/O failure, ...) without exception-driven control flow.
enum class Status {
    OK,
    NOT_FOUND,
    DUPLICATE_KEY,
    INVALID_SCHEMA,
    INVALID_TYPE,
    TABLE_NOT_FOUND,
    TABLE_ALREADY_EXISTS,
    IO_ERROR,
    PARSE_ERROR,
    OUT_OF_MEMORY,
    UNKNOWN_ERROR
};

// clang-format off
using RecordID  = uint64_t;    ///< Unique identifier for a record, scoped to its table.
using PageID    = uint32_t;    ///< Unique identifier for a storage page, scoped to its table.
using TableID   = uint32_t;    ///< Unique identifier for a table, scoped to its database.
using FieldName = std::string; ///< Name of a column/field.
// clang-format on

/// @brief Compile-time tunables shared across MiniDB's modules (storage
/// layout, cache sizing, concurrency, sentinel IDs). Centralized here so
/// every module sizes itself consistently without duplicating magic
/// numbers.
namespace DBConstants {
// clang-format off
constexpr uint32_t PAGE_SIZE         = 4096;    ///< Bytes per storage page.
constexpr uint32_t MAX_RECORDS_PAGE  = 64;      ///< Maximum records stored per page.
constexpr uint32_t MAX_TABLES        = 256;     ///< Maximum tables per database.
constexpr uint32_t LRU_CACHE_CAP     = 128;     ///< Pages kept resident in the LRU page cache.
constexpr uint32_t POOL_BLOCK_SIZE   = 256;     ///< PoolAllocator block size, in bytes.
constexpr uint32_t ARENA_SIZE        = 1 << 20; ///< Bytes per query Arena (1 MB).
constexpr uint32_t THREAD_POOL_SIZE  = 4;       ///< Default PulseThreadPool worker thread count.
constexpr RecordID INVALID_RECORD_ID = UINT64_MAX; ///< Sentinel for "no record".
constexpr PageID   INVALID_PAGE_ID   = UINT32_MAX; ///< Sentinel for "no page".
constexpr TableID  INVALID_TABLE_ID  = UINT32_MAX; ///< Sentinel for "no table".
// clang-format on
} // namespace DBConstants

/// @brief Describes a single column in a table schema: its name, value
/// type, and whether it may hold a null value.
struct ColumnDef {
    FieldName name;
    ColumnType type;
    bool nullable = false;
};

/**
 * @brief Renders a `Status` as a human-readable string, e.g. for logging
 * or error messages.
 * @param s Status to render.
 * @return A statically-allocated string naming `s`; `"UNKNOWN_ERROR"` for
 * any value not otherwise recognized.
 */
inline const char* statusToString(Status s) {
    switch (s) {
    case Status::OK:
        return "OK";
    case Status::NOT_FOUND:
        return "NOT_FOUND";
    case Status::DUPLICATE_KEY:
        return "DUPLICATE_KEY";
    case Status::INVALID_SCHEMA:
        return "INVALID_SCHEMA";
    case Status::INVALID_TYPE:
        return "INVALID_TYPE";
    case Status::TABLE_NOT_FOUND:
        return "TABLE_NOT_FOUND";
    case Status::TABLE_ALREADY_EXISTS:
        return "TABLE_ALREADY_EXISTS";
    case Status::IO_ERROR:
        return "IO_ERROR";
    case Status::PARSE_ERROR:
        return "PARSE_ERROR";
    case Status::OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    default:
        return "UNKNOWN_ERROR";
    }
}

} // namespace MiniDB::Common
