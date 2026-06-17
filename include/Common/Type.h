#pragma once 

#include <string>
#include <cstdint>

// Column Types
enum class ColumnType {
    INT,
    DOUBLE,
    STRING,
    BOOL
};

// Query Operators
enum class Op {
    EQ,    // ==
    NEQ,   // !=
    GT,    // >
    GTE,   // >=
    LT,    // <
    LTE    // <=
};

// Sort Direction
enum class SortOrder {
    ASC,
    DESC
};

// Operation Result
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

// Type Aliases 
using RecordID   = uint64_t;
using PageID     = uint32_t;
using TableID    = uint32_t;
using FieldName  = std::string;

// Constants
namespace DBConstants {
    constexpr uint32_t PAGE_SIZE          = 4096;       // bytes per page
    constexpr uint32_t MAX_RECORDS_PAGE   = 64;         // records per page
    constexpr uint32_t MAX_TABLES         = 256;        // tables per database
    constexpr uint32_t LRU_CACHE_CAP      = 128;        // pages kept in LRU cache
    constexpr uint32_t POOL_BLOCK_SIZE    = 256;        // PoolAllocator block size(bytes)
    constexpr uint32_t ARENA_SIZE         = 1 << 20;    // 1 MB per query Arena
    constexpr uint32_t THREAD_POOL_SIZE   = 4;          // default PulseThreadPool threads
    constexpr RecordID INVALID_RECORD_ID  = UINT64_MAX;
    constexpr PageID   INVALID_PAGE_ID    = UINT32_MAX;
    constexpr TableID  INVALID_TABLE_ID   = UINT32_MAX;
};

// Column Definition
struct ColumnDef {
    FieldName   name;
    ColumnType  type;
    bool        nullable = false;
};

// Helper 
inline const char* statusToString(Status s) {
    switch(s) {
        case Status::OK:                    return "OK";
        case Status::NOT_FOUND:             return "NOT_FOUND";
        case Status::DUPLICATE_KEY:         return "DUPLICATE_KEY";
        case Status::INVALID_SCHEMA:        return "INVALID_SCHEMA";
        case Status::INVALID_TYPE:          return "INVALID_TYPE";
        case Status::TABLE_NOT_FOUND:       return "TABLE_NOT_FOUND";
        case Status::TABLE_ALREADY_EXISTS:  return "TABLE_ALREADY_EXISTS";
        case Status::IO_ERROR:              return "IO_ERROR";
        case Status::PARSE_ERROR:           return "PARSE_ERROR";
        case Status::OUT_OF_MEMORY:         return "OUT_OF_MEMORY";
        default:                            return "UNKNOWN_ERROR";
    }
}



