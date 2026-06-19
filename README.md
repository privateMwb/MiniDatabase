# MiniDatabase

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-learning%20project-green)](https://github.com/privateMwb/MiniDatabase)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A lightweight embedded database engine built entirely from custom C++23 libraries — no STL containers, no third-party dependencies. Every subsystem, from memory allocation to JSON parsing to concurrent execution, is built from scratch and assembled into a single working engine.

---

## Table of Contents

- [Overview](#overview)
- [Motivation](#motivation)
- [Features](#features)
- [Design Overview](#design-overview)
  - [Architecture](#architecture)
  - [Phase Breakdown](#phase-breakdown)
  - [Custom Libraries Used](#custom-libraries-used)
  - [Schema Validation](#schema-validation)
  - [Tombstone Deletion & Compaction](#tombstone-deletion--compaction)
  - [Page & Index Design](#page--index-design)
  - [Caching Strategy](#caching-strategy)
  - [Serialization Formats](#serialization-formats)
  - [Concurrency Model](#concurrency-model)
- [Complexity](#complexity)
- [Quick Example](#quick-example)
- [Core API](#core-api)
- [Benchmark Results](#benchmark-results)
- [Project Structure](#project-structure)
- [Build Instructions](#build-instructions)
- [Notes / Known Limitations](#notes--known-limitations)
- [License](#license)

---

## Overview

MiniDatabase is a tiny, embedded, table-based database engine. It supports table creation with a dynamic runtime schema, full CRUD operations, predicate-based querying with sorting and aggregates, disk persistence, page-level caching, JSON import/export, and parallel multi-table operations — all without depending on `std::vector`, `std::unordered_map`, or any other STL container for its core data structures.

Records are stored as `Json` objects with a validated schema, giving the flexibility of a document store with the safety of typed columns.

---

## Motivation

This project exists to demonstrate systems-level engineering, not just algorithmic knowledge. The goal was to take a set of previously built, independent libraries — a vector, a hash map, a JSON parser, a thread pool, an LRU cache, and custom memory allocators — and integrate all of them into one cohesive, production-style application.

It answers a simple question: can these pieces, built independently, actually work together to form something real?

---

## Features

- Dynamic runtime schema definition with typed, nullable columns
- Full CRUD: insert, update, delete, select
- Predicate-based filtering with `AND`-combined conditions
- Sorting (ascending/descending) and aggregates (`count`, `sum`, `avg`, `max`, `min`)
- Tombstone-based deletion with explicit compaction
- Multi-page tables with automatic page allocation on overflow
- O(1) average record lookup via HashMap index
- Full database persistence (`save`/`load`) with index rebuild on load
- Page-level LRU caching with built-in hit/miss statistics
- Human-readable JSON export/import per table or per database
- Parallel save, load, index rebuild, and export via a custom thread pool
- Zero STL containers in the core engine — `VectorPro` and `HashMap` throughout

---

## Design Overview

### Architecture

```
MiniDatabase
     │
     Database Engine
     │
 ┌───────────┬───────────┬───────────┐
 │           │           │
Storage     Query      Serialization
 │           │             │
 │           │         JsonParser
 │           │
VectorPro  HashMap
 │           │
 └─────┬─────┘
       │
Custom Allocators
       │
   LRU Cache
       │
PulseThreadPool
```

### Phase Breakdown

| Phase | Goal | Components |
|-------|------|-------------|
| 1 — Foundation | Core primitives | `Types`, `Record`, `Page`, `Table` |
| 2 — Database Core | Working in-memory database | `Database`, `QueryEngine` |
| 3 — Storage & Caching | Persistence and performance | `StorageEngine`, `LRUCache` integration |
| 4 — Serialization | JSON import/export | `Serializer` |
| 5 — Concurrency | Parallel operations | `Concurrency`, `PulseThreadPool` integration |
| 6 — Polish | Verification and demonstration | `tests/`, `benchmarks/`, `examples/` |

### Custom Libraries Used

| Library | Role in MiniDatabase | Repository |
|---------|----------------------|-------------|
| VectorPro | Table row storage, page lists, schema storage | [VectorPro](https://github.com/privateMwb/VectorPro) |
| HashMap | Record ID → page index, table name index | [HashMap](https://github.com/privateMwb/HashMap) |
| JsonParser | Record storage format, serialization, import/export | [JsonParser](https://github.com/privateMwb/JsonParser) |
| LRUCache | Page-level caching in StorageEngine | [LRUCache](https://github.com/privateMwb/LRUCache) |
| PulseThreadPool | Parallel save/load/export/index-rebuild | [Pulse-Thread-Pool](https://github.com/privateMwb/Pulse-Thread-Pool) |
| Custom Allocators | PoolAllocator (record slots), ArenaAllocator (query scratch space) | [Custom-Allocators](https://github.com/privateMwb/Custom-Allocators) |

### Schema Validation

Every table is created with a `VectorPro<ColumnDef>` schema — each column has a name, a `ColumnType` (`INT`, `DOUBLE`, `STRING`, `BOOL`), and a nullable flag. Every insert and update runs `Record::validate()` against this schema before anything is written, rejecting missing required fields (`INVALID_SCEMA`) or mismatched types (`INVALID_TYPE`).

### Tombstone Deletion & Compaction

Deleting a record never immediately frees its memory. `Page::deleteRecord()` marks the record with a `deleted` flag — the record becomes invisible to all queries immediately, but its physical slot remains until `Page::compact()` runs, which sweeps every tombstoned record, returns its slot to the `PoolAllocator`, and rebuilds the page's live record list.

### Page & Index Design

Each `Table` holds a `VectorPro<Page*>` and a `HashMap<RecordID, PageID>` index. Insertion finds (or allocates) a page with a free slot; lookups go straight through the index to the owning page in O(1) average time, rather than scanning every page.

### Caching Strategy

`StorageEngine` wraps an `LRUCache<PageID, Page>` between the engine and disk. Reads check the cache first; on a cache hit, no disk access occurs. The cache reports its own hit/miss counters and hit rate via `cacheHitRate()`, `cacheHits()`, and `cacheMisses()`.

### Serialization Formats

MiniDatabase has three distinct export/import paths, each suited to a different use case:

| Path | Format | Scope | Use case |
|------|--------|-------|----------|
| `Database::save` / `Database::load` | Internal, full metadata | Whole database | Reloading MiniDatabase's own state exactly |
| `Serializer::exportTableToFile` / `importTableFromFile` | Clean, human-readable | One table | Backups, external tools, sharing |
| `Serializer::exportDatabaseToJson` / `importDatabaseFromJson` | Clean, human-readable | Whole database | Multi-table human-readable backup |

`Concurrency::exportAllTablesParallel` uses the same format as the second row, parallelized across tables via the thread pool.

### Concurrency Model

`Concurrency` wraps a `PulseThreadPool` and exposes parallel versions of save, load, index rebuild, and export — one task per table, awaited via `std::future`. Tables are only ever touched by their own task, so no locking is required between concurrent operations.

---

## Complexity

| Operation | Complexity | Notes |
|-----------|:----------:|-------|
| `Table::insertRecord` | O(1) amortized | Page lookup/allocation + HashMap insert |
| `Table::getRecord` | O(1) average | HashMap index lookup |
| `Table::updateRecord` | O(1) average | Index lookup + in-place overwrite |
| `Table::deleteRecord` | O(1) average | Index lookup + tombstone flag |
| `QueryEngine::selectById` | O(1) average | Delegates to `Table::getRecord` |
| `QueryEngine::selectAll` / `select` | O(n) | Full scan across all pages |
| `QueryEngine::count` / `sum` / `avg` / `max` / `min` | O(n) | Full scan with predicate evaluation |
| `Page::compact` | O(p) | p = records on the page |
| `Table::rebuildIndex` | O(n) | Full scan, rebuilds HashMap from scratch |

---

## Quick Example

```cpp
#include "Database.h"
#include "QueryEngine.h"
#include "ArenaAllocator.h"

// Schema
VectorPro<ColumnDef> schema;
schema.push_back({"name", ColumnType::STRING, false});
schema.push_back({"salary", ColumnType::DOUBLE, false});

// Database and table
Database db("CompanyDB");
db.createTable("employees", schema);
Table* employees = db.getTable("employees");

// Insert
Json fields(Json::ObjectType{});
fields["name"]   = "Alice";
fields["salary"] = 85000;
employees->insertRecord(Record(1, fields));

// Query
ArenaAllocator arena(DBConstants::ARENA_SIZE);
QueryEngine qe(arena);

FilterPredicate filter{"salary", Op::GT, Json(50000.0)};
VectorPro<FilterPredicate*> predicates;
predicates.push_back(&filter);

QueryResult result = qe.select(*employees, predicates);

// Persistence
db.save("company.json");
```

See `examples/employees.cpp`, `examples/students.cpp`, and `examples/products.cpp` for complete, runnable programs covering querying, sorting, aggregates, persistence, JSON export/import, and parallel operations.

---

## Core API

### Database
`createTable`, `dropTable`, `getTable`, `hasTable`, `save`, `load`, `compact`, `recordCount`, `tableCount`

### Table
`insertRecord`, `getRecord`, `updateRecord`, `deleteRecord`, `compact`, `rebuildIndex`, `serialize`, `deserialize`

### Record
`setField`, `getField`, `hasField`, `removeField`, `validate`, `serialize`, `deserialize`, `markDeleted`

### QueryEngine
`select`, `selectAll`, `selectById`, `count`, `sum`, `avg`, `max`, `min`

### StorageEngine
`saveDatabase`, `loadDatabase`, `getCachedPage`, `cachePage`, `isCached`, `evictPage`, `cacheHitRate`

### Serializer
`exportTableToJson`, `importTableFromJson`, `exportTableToFile`, `importTableFromFile`, `exportDatabaseToJson`, `importDatabaseFromJson`

### Concurrency
`saveAllTablesParallel`, `loadAllTablesParallel`, `rebuildAllIndexesParallel`, `exportAllTablesParallel`

---

## Benchmark Results

All benchmarks compiled with `-O2`. Full raw output available under `benchmarks/`.

### Insert Throughput

| Records | Time | Rate |
|---------|------|------|
| 1,000 | 19,843 µs | 50,395 rec/s |
| 10,000 | 196,316 µs | 50,938 rec/s |
| 100,000 | 4,333,115 µs | 23,078 rec/s |

Throughput holds steady up to 10,000 records, then drops at 100,000 — consistent with the table spanning many pages and `findPageWithSlot()`'s linear scan for an open page becoming more expensive as page count grows.

### Average Insert Cost

| Records | Avg Cost |
|---------|----------|
| 1,000 | 17,323 ns/insert |
| 10,000 | 19,365 ns/insert |
| 100,000 | 168,937 ns/insert |

### Select By Id (HashMap Index) — 10,000 lookups

| Table Size | Time |
|------------|------|
| 1,000 | 179,259 µs |
| 10,000 | 208,626 µs |
| 100,000 | 207,674 µs |

Lookup time stays roughly flat from 10,000 to 100,000 records — the expected signature of an O(1) average-case index lookup, regardless of table size.

### Select All (Full Scan)

| Table Size | Time |
|------------|------|
| 1,000 | 14,089 µs |
| 10,000 | 123,566 µs |
| 100,000 | 1,439,153 µs |

Scan time grows roughly linearly with table size, as expected for an O(n) full scan — contrasting directly with the flat `selectById` numbers above.

### Filtered Select (`salary > threshold`)

| Table Size | Time |
|------------|------|
| 1,000 | 9,986 µs |
| 10,000 | 141,453 µs |
| 100,000 | 1,094,077 µs |

### Aggregates — 100,000 records

| Aggregate | Time |
|-----------|------|
| sum | 443,300 µs |
| avg | 417,384 µs |
| max | 414,529 µs |
| min | 416,977 µs |

All four aggregates cost roughly the same, as expected — each is a single O(n) scan with a different running calculation per record.

### Cached vs Uncached Page Access — 1,000 ops

| Access Type | Time |
|-------------|------|
| Cached (hit) | 366 µs |
| Uncached (simulated disk) | 8,210,420 µs |

The cached path is roughly 22,000× faster than the simulated disk path in this benchmark, demonstrating the practical value of the LRU page cache when disk latency is involved.

### Cache Hit Rate — Working Set Fits in Cache

| Metric | Value |
|--------|-------|
| Hits | 1,000 |
| Misses | 0 |
| Hit Rate | 100% |

### Cache Hit Rate — Working Set Exceeds Capacity

| Metric | Value |
|--------|-------|
| Hits | 1,000 |
| Misses | 0 |
| Hit Rate | 100% |

> **Note:** this result is suspected to be a bug rather than genuine cache behavior. A working set of `2 × LRU_CACHE_CAP` accessed sequentially should produce a meaningful number of misses as older pages are evicted. A 100% hit rate with zero misses here most likely indicates an issue in either the benchmark's cache-fill logic or in `LRUCache`'s eviction path under this access pattern, and is flagged here rather than presented as a genuine result. See [Known Limitations](#notes--known-limitations).

### Export Throughput

| Records | Time | Rate |
|---------|------|------|
| 1,000 | 45,128 µs | 22,159 rec/s |
| 10,000 | 657,169 µs | 15,216 rec/s |
| 100,000 | 4,597,666 µs | 21,750 rec/s |

### Import Throughput

| Records | Time | Rate |
|---------|------|------|
| 1,000 | 80,445 µs | 12,430 rec/s |
| 10,000 | 843,439 µs | 11,856 rec/s |
| 100,000 | 14,214,954 µs | 7,034 rec/s |

Import is consistently slower than export at every scale — expected, since import additionally performs schema validation and HashMap index insertion per record via `Table::insertRecord`, while export only reads and serializes existing data.

### Round Trip (Export + Import)

| Records | Time |
|---------|------|
| 1,000 | 139,061 µs |
| 10,000 | 1,388,359 µs |
| 100,000 | 19,107,715 µs |

### Summary

- **Index lookups are genuinely O(1) average-case** — flat across two orders of magnitude of table size, in clear contrast to the linear growth of full scans.
- **The page cache provides a substantial, measurable speedup** over simulated disk access.
- **Import is consistently more expensive than export**, due to schema validation and index maintenance that export doesn't need to perform.
- **Insert and import throughput both degrade at 100,000 records**, suggesting page-scan and index-maintenance costs that would benefit from further optimization at larger scales.

---

## Project Structure

```
MiniDatabase/
├── include/
│   ├── Core/
│   │   ├── Record.h
│   │   ├── Page.h
│   │   ├── Table.h
│   │   └── Database.h
│   ├── Engine/
│   │   ├── QueryEngine.h
│   │   ├── StorageEngine.h
│   │   ├── Serializer.h
│   │   └── Concurrency.h
│   └── Common/
│       └── Types.h
│
├── src/
│   ├── Core/
│   │   ├── Record.cpp
│   │   ├── Page.cpp
│   │   ├── Table.cpp
│   │   └── Database.cpp
│   └── Engine/
│       ├── QueryEngine.cpp
│       ├── StorageEngine.cpp
│       ├── Serializer.cpp
│       └── Concurrency.cpp
│
├── libs/
│   ├── VectorPro/
│   ├── HashMap/
│   ├── JsonParser/
│   ├── PulseThreadPool/
│   ├── LRUCache/
│   └── CustomAllocators/
│       ├── ArenaAllocator/
│       └── PoolAllocator/
│
├── tests/
│   ├── TestHelpers.h
│   ├── main.cpp
│   ├── test_record.cpp
│   ├── test_page.cpp
│   ├── test_table.cpp
│   ├── test_database.cpp
│   ├── test_query.cpp
│   ├── test_storage.cpp
│   ├── test_serializer.cpp
│   └── test_concurrency.cpp
│
├── benchmarks/
│   ├── BenchTable.h
│   ├── bench_insert.cpp
│   ├── bench_query.cpp
│   ├── bench_storage.cpp
│   └── bench_serializer.cpp
│
├── examples/
│   ├── employees.cpp
│   ├── students.cpp
│   ├── products.cpp
│   └── exported/
│
├── README.md
└── LICENSE
```

---

## Build Instructions

All commands assume you are in the `MiniDatabase/` project root.

### Running the test suite

```bash
g++ -std=c++23 \
    tests/*.cpp \
    src/Core/*.cpp \
    src/Engine/*.cpp \
    libs/JsonParser/*.cpp \
    libs/PulseThreadPool/*.cpp \
    -I include -I libs \
    -lpthread \
    -o run_tests

./run_tests
```

### Running a benchmark

```bash
g++ -std=c++23 -O2 \
    benchmarks/bench_insert.cpp \
    src/Core/Record.cpp \
    src/Core/Page.cpp \
    src/Core/Table.cpp \
    libs/JsonParser/Json.cpp \
    libs/JsonParser/Parser.cpp \
    -I include -I libs \
    -o bench_insert

./bench_insert
```

Repeat with `bench_query.cpp`, `bench_storage.cpp`, or `bench_serializer.cpp`, adjusting the linked source files to match each benchmark's dependencies (see each file's header comment).

### Running an example

```bash
g++ -std=c++23 -O2 \
    examples/employees.cpp \
    src/Core/Record.cpp \
    src/Core/Page.cpp \
    src/Core/Table.cpp \
    src/Core/Database.cpp \
    src/Engine/QueryEngine.cpp \
    libs/JsonParser/Json.cpp \
    libs/JsonParser/Parser.cpp \
    -I include -I libs \
    -o employees_example

./employees_example
```

`students.cpp` additionally requires `src/Engine/Serializer.cpp`. `products.cpp` additionally requires `src/Engine/Concurrency.cpp`, `libs/PulseThreadPool/ThreadPool.cpp`, and `-lpthread`.

> **Note:** examples that write files (`students.cpp`, `products.cpp`) expect an `examples/exported/` directory to already exist on disk. Create it first with `mkdir -p examples/exported` if it isn't present.

---

## Notes / Known Limitations

- **StackAllocator and FreeListAllocator were built but never wired in.** Both were part of the original library set but ended up unused in the final design — deletion uses tombstoning rather than a free list, and no part of the engine required stack-discipline allocation. They remain available as standalone libraries.
- **The eviction-pressure cache benchmark currently reports a 100% hit rate**, which is almost certainly incorrect for a working set twice the cache's capacity. This is flagged as a known issue rather than presented as genuine cache behavior — likely a benchmark setup issue or an `LRUCache` eviction edge case worth investigating further.
- **`Table::findPageWithSlot()` performs a linear scan** over all pages to find one with a free slot before allocating a new page. This is the likely cause of the insert-throughput drop-off observed at 100,000 records, and would benefit from a free-page tracking structure at larger scales.
- **Numeric columns (`INT` vs `DOUBLE`) are both backed by `Json`'s single `double`-based number type**, since the underlying JSON value model does not distinguish integers from floating-point numbers. `Record::validate()` approximates an `INT` check by confirming the value has no fractional component.
- **`Serializer::importTableFromJson`/`importDatabaseFromJson` only populate existing tables** — they never create new ones. The caller is responsible for calling `createTable` with a matching schema first.

---

## License

MIT License. See `LICENSE` for details.
