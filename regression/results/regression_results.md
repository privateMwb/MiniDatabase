#MiniDBRegression Report

## Page Access

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| get record last slot | 10K | 0 ns | 204 ns | +inf% |
| get record last slot | 100K | 0 ns | 204 ns | +inf% |
| get record last slot | 1M | 0 ns | 218 ns | +inf% |
| get record at last slot | 10K | 0 ns | 2 ns | +inf% |
| get record at last slot | 100K | 0 ns | 2 ns | +inf% |
| get record at last slot | 1M | 0 ns | 2 ns | +inf% |
| get record id miss | 10K | 0 ns | 206 ns | +inf% |
| get record id miss | 100K | 0 ns | 208 ns | +inf% |
| get record id miss | 1M | 0 ns | 213 ns | +inf% |
| get record at oor | 10K | 0 ns | 2 ns | +inf% |
| get record at oor | 100K | 0 ns | 2 ns | +inf% |
| get record at oor | 1M | 0 ns | 2 ns | +inf% |

## Query Engine Select

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| QueryEngine selectAll | 1M | 0 ns | 507.42 us | +inf% |
| QueryEngine selectAll | 1M | 0 ns | 496.33 us | +inf% |
| QueryEngine selectAll | 10K | 0 ns | 493.36 us | +inf% |
| QueryEngine select(eq) | 1M | 0 ns | 160.65 us | +inf% |
| QueryEngine select(eq) | 1M | 0 ns | 160.30 us | +inf% |
| QueryEngine select(eq) | 10K | 0 ns | 160.39 us | +inf% |
| QueryEngine select(sort) | 1M | 0 ns | 2.66 ms | +inf% |
| QueryEngine select(sort) | 1M | 0 ns | 2.65 ms | +inf% |
| QueryEngine select(sort) | 10K | 0 ns | 2.65 ms | +inf% |

## Storage Engine Cache

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| fetch page hit | 10K | 0 ns | 56 ns | +inf% |
| fetch page hit | 100K | 0 ns | 57 ns | +inf% |
| fetch page hit | 1M | 0 ns | 59 ns | +inf% |
| fetch page miss | 10K | 0 ns | 5.16 us | +inf% |
| fetch page miss | 100K | 0 ns | 5.17 us | +inf% |
| fetch page miss | 1M | 0 ns | 5.18 us | +inf% |
| get cached page | 10K | 0 ns | 45 ns | +inf% |
| get cached page | 100K | 0 ns | 49 ns | +inf% |
| get cached page | 1M | 0 ns | 48 ns | +inf% |

## Table Access

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| getRecord (single page) | 10K | 0 ns | 67 ns | +inf% |
| getRecord (single page) | 100K | 0 ns | 68 ns | +inf% |
| getRecord (single page) | 1M | 0 ns | 68 ns | +inf% |
| getRecord (multi page) | 10K | 0 ns | 23 ns | +inf% |
| getRecord (multi page) | 100K | 0 ns | 22 ns | +inf% |
| getRecord (multi page) | 1M | 0 ns | 23 ns | +inf% |
| getRecord (miss) | 10K | 0 ns | 2 ns | +inf% |
| getRecord (miss) | 100K | 0 ns | 2 ns | +inf% |
| getRecord (miss) | 1M | 0 ns | 2 ns | +inf% |

## Database Table Ops

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| Database createTable | 10K | 0 ns | 296 ns | +inf% |
| Database createTable | 100K | 0 ns | 289 ns | +inf% |
| Database createTable | 1M | 0 ns | 289 ns | +inf% |
| Database dropTable | 10K | 0 ns | 301 ns | +inf% |
| Database dropTable | 100K | 0 ns | 301 ns | +inf% |
| Database dropTable | 1M | 0 ns | 300 ns | +inf% |
| Database getTable | 10K | 0 ns | 29 ns | +inf% |
| Database getTable | 100K | 0 ns | 28 ns | +inf% |
| Database getTable | 1M | 0 ns | 28 ns | +inf% |
| Database hasTable | 10K | 0 ns | 15 ns | +inf% |
| Database hasTable | 100K | 0 ns | 16 ns | +inf% |
| Database hasTable | 1M | 0 ns | 15 ns | +inf% |

## Page Mutation

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| addRecord(fill page) | 10K | 0 ns | 3.63 us | +inf% |
| addRecord(fill page) | 100K | 0 ns | 3.64 us | +inf% |
| addRecord(fill page) | 1M | 0 ns | 3.64 us | +inf% |
| updateRecord | 10K | 0 ns | 32 ns | +inf% |
| updateRecord | 100K | 0 ns | 31 ns | +inf% |
| updateRecord | 1M | 0 ns | 32 ns | +inf% |
| deleteRecord | 10K | 0 ns | 131 ns | +inf% |
| deleteRecord | 100K | 0 ns | 130 ns | +inf% |
| deleteRecord | 1M | 0 ns | 130 ns | +inf% |
| compact(half deleted) | 10K | 0 ns | 6.34 us | +inf% |
| compact(half deleted) | 100K | 0 ns | 6.34 us | +inf% |
| compact(half deleted) | 1M | 0 ns | 6.35 us | +inf% |

## Record Field

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| setField | 10K | 0 ns | 30 ns | +inf% |
| setField | 100K | 0 ns | 30 ns | +inf% |
| setField | 1M | 0 ns | 30 ns | +inf% |
| getField | 10K | 0 ns | 34 ns | +inf% |
| getField | 100K | 0 ns | 33 ns | +inf% |
| getField | 1M | 0 ns | 33 ns | +inf% |
| getFieldRef | 10K | 0 ns | 28 ns | +inf% |
| getFieldRef | 100K | 0 ns | 29 ns | +inf% |
| getFieldRef | 1M | 0 ns | 29 ns | +inf% |
| hasField | 10K | 0 ns | 17 ns | +inf% |
| hasField | 100K | 0 ns | 23 ns | +inf% |
| hasField | 1M | 0 ns | 14 ns | +inf% |
| removeField | 10K | 0 ns | 123 ns | +inf% |
| removeField | 100K | 0 ns | 121 ns | +inf% |
| removeField | 1M | 0 ns | 121 ns | +inf% |

## Table Mutation

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| Table insertRecord | 1M | 0 ns | 109 ns | +inf% |
| Table insertRecord | 1M | 0 ns | 128 ns | +inf% |
| Table insertRecord | 10K | 0 ns | 288 ns | +inf% |
| Table updateRecord | 10K | 0 ns | 41 ns | +inf% |
| Table updateRecord | 100K | 0 ns | 42 ns | +inf% |
| Table updateRecord | 1M | 0 ns | 41 ns | +inf% |
| Table deleteRecord | 10K | 0 ns | 7 ns | +inf% |
| Table deleteRecord | 100K | 0 ns | 2 ns | +inf% |
| Table deleteRecord | 1M | 0 ns | 2 ns | +inf% |
| Table rebuildIndex | 1M | 0 ns | 17.01 us | +inf% |
| Table rebuildIndex | 1M | 0 ns | 17.05 us | +inf% |
| Table rebuildIndex | 10K | 0 ns | 17.03 us | +inf% |

## Database

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| db ctor | 10K | 0 ns | 61 ns | +inf% |
| db ctor | 100K | 0 ns | 79 ns | +inf% |
| db ctor | 1M | 0 ns | 62 ns | +inf% |
| db move ctor(empty) | 10K | 0 ns | 70 ns | +inf% |
| db move ctor(empty) | 100K | 0 ns | 71 ns | +inf% |
| db move ctor(empty) | 1M | 0 ns | 71 ns | +inf% |
| db move ctor(50 tables) | 10K | 0 ns | 12.84 us | +inf% |
| db move ctor(50 tables) | 100K | 0 ns | 12.84 us | +inf% |
| db move ctor(50 tables) | 1M | 0 ns | 12.85 us | +inf% |

## Page

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| Page default ctor | 10K | 0 ns | 49 ns | +inf% |
| Page default ctor | 100K | 0 ns | 48 ns | +inf% |
| Page default ctor | 1M | 0 ns | 48 ns | +inf% |
| Page id ctor | 10K | 0 ns | 47 ns | +inf% |
| Page id ctor | 100K | 0 ns | 48 ns | +inf% |
| Page id ctor | 1M | 0 ns | 48 ns | +inf% |
| Page move ctor (empty) | 10K | 0 ns | 58 ns | +inf% |
| Page move ctor (empty) | 100K | 0 ns | 56 ns | +inf% |
| Page move ctor (empty) | 1M | 0 ns | 56 ns | +inf% |
| Page move ctor(full) | 10K | 0 ns | 3.64 us | +inf% |
| Page move ctor(full) | 100K | 0 ns | 3.71 us | +inf% |
| Page move ctor(full) | 1M | 0 ns | 3.81 us | +inf% |

## Record

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| Record default ctor | 10K | 0 ns | 20 ns | +inf% |
| Record default ctor | 100K | 0 ns | 21 ns | +inf% |
| Record default ctor | 1M | 0 ns | 21 ns | +inf% |
| Record id ctor | 10K | 0 ns | 21 ns | +inf% |
| Record id ctor | 100K | 0 ns | 21 ns | +inf% |
| Record id ctor | 1M | 0 ns | 21 ns | +inf% |
| Record id+data ctor | 10K | 0 ns | 131 ns | +inf% |
| Record id+data ctor | 100K | 0 ns | 137 ns | +inf% |
| Record id+data ctor | 1M | 0 ns | 133 ns | +inf% |
| Record copy ctor | 10K | 0 ns | 110 ns | +inf% |
| Record copy ctor | 100K | 0 ns | 111 ns | +inf% |
| Record copy ctor | 1M | 0 ns | 110 ns | +inf% |
| Record move ctor | 10K | 0 ns | 264 ns | +inf% |
| Record move ctor | 100K | 0 ns | 264 ns | +inf% |
| Record move ctor | 1M | 0 ns | 261 ns | +inf% |

## Table

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| ctor(empty schema) | 10K | 0 ns | 69 ns | +inf% |
| ctor(empty schema) | 100K | 0 ns | 86 ns | +inf% |
| ctor(empty schema) | 1M | 0 ns | 69 ns | +inf% |
| ctor(20-col schema) | 10K | 0 ns | 226 ns | +inf% |
| ctor(20-col schema) | 100K | 0 ns | 220 ns | +inf% |
| ctor(20-col schema) | 1M | 0 ns | 220 ns | +inf% |
| move ctor(empty) | 10K | 0 ns | 76 ns | +inf% |
| move ctor(empty) | 100K | 0 ns | 76 ns | +inf% |
| move ctor(empty) | 1M | 0 ns | 77 ns | +inf% |
| move ctor(populated) | 1M | 0 ns | 38.99 us | +inf% |
| move ctor(populated) | 1M | 0 ns | 38.40 us | +inf% |
| move ctor(populated) | 10K | 0 ns | 38.51 us | +inf% |

## Concurrency Scaling

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| rebuild(2 tables) | 1M | 0 ns | 11.78 us | +inf% |
| rebuild(2 tables) | 1M | 0 ns | 7.13 us | +inf% |
| rebuild(2 tables) | 10K | 0 ns | 7.12 us | +inf% |
| rebuild(4 tables) | 1M | 0 ns | 11.00 us | +inf% |
| rebuild(4 tables) | 1M | 0 ns | 13.06 us | +inf% |
| rebuild(4 tables) | 10K | 0 ns | 11.58 us | +inf% |
| rebuild(16 tables) | 1M | 0 ns | 37.83 us | +inf% |
| rebuild(16 tables) | 1M | 0 ns | 39.44 us | +inf% |
| rebuild(16 tables) | 10K | 0 ns | 38.39 us | +inf% |
| rebuild(64 tables) | 1M | 0 ns | 172.10 us | +inf% |
| rebuild(64 tables) | 1M | 0 ns | 143.00 us | +inf% |
| rebuild(64 tables) | 10K | 0 ns | 142.89 us | +inf% |

## Database Scaling

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| db createTable(10) | 10K | 0 ns | 212 ns | +inf% |
| db createTable(10) | 100K | 0 ns | 210 ns | +inf% |
| db createTable(10) | 1M | 0 ns | 210 ns | +inf% |
| db createTable(100) | 10K | 0 ns | 437 ns | +inf% |
| db createTable(100) | 100K | 0 ns | 436 ns | +inf% |
| db createTable(100) | 1M | 0 ns | 436 ns | +inf% |
| db createTable(200) | 10K | 0 ns | 633 ns | +inf% |
| db createTable(200) | 100K | 0 ns | 632 ns | +inf% |
| db createTable(200) | 1M | 0 ns | 633 ns | +inf% |
| db getTable(10) | 10K | 0 ns | 27 ns | +inf% |
| db getTable(10) | 100K | 0 ns | 27 ns | +inf% |
| db getTable(10) | 1M | 0 ns | 27 ns | +inf% |
| db getTable(100) | 10K | 0 ns | 21 ns | +inf% |
| db getTable(100) | 100K | 0 ns | 21 ns | +inf% |
| db getTable(100) | 1M | 0 ns | 21 ns | +inf% |
| db getTable(200) | 10K | 0 ns | 21 ns | +inf% |
| db getTable(200) | 100K | 0 ns | 22 ns | +inf% |
| db getTable(200) | 1M | 0 ns | 21 ns | +inf% |

## QueryEngine

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| qe select eq(1) | 1M | 0 ns | 3.27 us | +inf% |
| qe select eq(1) | 1M | 0 ns | 3.21 us | +inf% |
| qe select eq(1) | 10K | 0 ns | 3.19 us | +inf% |
| qe select eq(100) | 1M | 0 ns | 322.31 us | +inf% |
| qe select eq(100) | 1M | 0 ns | 322.45 us | +inf% |
| qe select eq(100) | 10K | 0 ns | 322.58 us | +inf% |
| qe select eq(1000) | 1M | 0 ns | 3.56 ms | +inf% |
| qe select eq(1000) | 1M | 0 ns | 3.55 ms | +inf% |
| qe select eq(1000) | 10K | 0 ns | 3.59 ms | +inf% |
| qe select sorted(1) | 1M | 0 ns | 30.83 us | +inf% |
| qe select sorted(1) | 1M | 0 ns | 30.05 us | +inf% |
| qe select sorted(1) | 10K | 0 ns | 30.07 us | +inf% |
| qe select sorted(100) | 1M | 0 ns | 5.67 ms | +inf% |
| qe select sorted(100) | 1M | 0 ns | 5.74 ms | +inf% |
| qe select sorted(100) | 10K | 0 ns | 5.67 ms | +inf% |
| qe select sorted(1000) | 1M | 0 ns | 69.21 ms | +inf% |
| qe select sorted(1000) | 1M | 0 ns | 68.84 ms | +inf% |
| qe select sorted(1000) | 10K | 0 ns | 68.73 ms | +inf% |

## Table Scaling

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| insertRecord(1 page) | 1M | 0 ns | 105 ns | +inf% |
| insertRecord(1 page) | 1M | 0 ns | 110 ns | +inf% |
| insertRecord(1 page) | 10K | 0 ns | 273 ns | +inf% |
| insertRecord(100 pages) | 1M | 0 ns | 278 ns | +inf% |
| insertRecord(100 pages) | 1M | 0 ns | 290 ns | +inf% |
| insertRecord(100 pages) | 10K | 0 ns | 460 ns | +inf% |
| insertRecord(1000 pages) | 1M | 0 ns | 1.98 us | +inf% |
| insertRecord(1000 pages) | 1M | 0 ns | 1.98 us | +inf% |
| insertRecord(1000 pages) | 10K | 0 ns | 2.14 us | +inf% |
| getRecord(1 page) | 10K | 0 ns | 23 ns | +inf% |
| getRecord(1 page) | 100K | 0 ns | 22 ns | +inf% |
| getRecord(1 page) | 1M | 0 ns | 23 ns | +inf% |
| getRecord(100 pages) | 10K | 0 ns | 23 ns | +inf% |
| getRecord(100 pages) | 100K | 0 ns | 22 ns | +inf% |
| getRecord(100 pages) | 1M | 0 ns | 22 ns | +inf% |
| getRecord(1000 pages) | 10K | 0 ns | 22 ns | +inf% |
| getRecord(1000 pages) | 100K | 0 ns | 22 ns | +inf% |
| getRecord(1000 pages) | 1M | 0 ns | 22 ns | +inf% |

## Concurrency

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| saveAllTablesParallel | 1M | 0 ns | 1.95 ms | +inf% |
| saveAllTablesParallel | 1M | 0 ns | 1.85 ms | +inf% |
| saveAllTablesParallel | 10K | 0 ns | 1.82 ms | +inf% |
| loadAllTablesParallel | 1M | 0 ns | 279.21 us | +inf% |
| loadAllTablesParallel | 1M | 0 ns | 281.52 us | +inf% |
| loadAllTablesParallel | 10K | 0 ns | 285.11 us | +inf% |
| rebuildAllIndexesParallel | 1M | 0 ns | 19.76 us | +inf% |
| rebuildAllIndexesParallel | 1M | 0 ns | 19.42 us | +inf% |
| rebuildAllIndexesParallel | 10K | 0 ns | 20.58 us | +inf% |
| exportAllTablesParallel | 1M | 0 ns | 1.80 ms | +inf% |
| exportAllTablesParallel | 1M | 0 ns | 1.80 ms | +inf% |
| exportAllTablesParallel | 10K | 0 ns | 1.80 ms | +inf% |

## File IO

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| FileIO writeSlot | 10K | 0 ns | 4.14 us | +inf% |
| FileIO writeSlot | 100K | 0 ns | 4.13 us | +inf% |
| FileIO writeSlot | 1M | 0 ns | 4.14 us | +inf% |
| FileIO readSlot | 10K | 0 ns | 3.64 us | +inf% |
| FileIO readSlot | 100K | 0 ns | 3.62 us | +inf% |
| FileIO readSlot | 1M | 0 ns | 3.62 us | +inf% |
| FileIO writeFileAtomic | 1M | 0 ns | 392.30 us | +inf% |
| FileIO writeFileAtomic | 1M | 0 ns | 386.63 us | +inf% |
| FileIO writeFileAtomic | 10K | 0 ns | 393.01 us | +inf% |
| FileIO readFile | 10K | 0 ns | 6.11 us | +inf% |
| FileIO readFile | 100K | 0 ns | 6.01 us | +inf% |
| FileIO readFile | 1M | 0 ns | 6.00 us | +inf% |

## Json Round Trip

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| Record to/fromJson | 10K | 0 ns | 667 ns | +inf% |
| Record to/fromJson | 100K | 0 ns | 664 ns | +inf% |
| Record to/fromJson | 1M | 0 ns | 665 ns | +inf% |
| Record serial/deserial | 10K | 0 ns | 2.23 us | +inf% |
| Record serial/deserial | 100K | 0 ns | 2.23 us | +inf% |
| Record serial/deserial | 1M | 0 ns | 2.23 us | +inf% |
| Page to/fromJson | 1M | 0 ns | 58.38 us | +inf% |
| Page to/fromJson | 1M | 0 ns | 57.56 us | +inf% |
| Page to/fromJson | 10K | 0 ns | 57.51 us | +inf% |
| Page serial/deserial | 1M | 0 ns | 121.37 us | +inf% |
| Page serial/deserial | 1M | 0 ns | 120.99 us | +inf% |
| Page serial/deserial | 10K | 0 ns | 121.24 us | +inf% |
| Table to/fromJson | 1M | 0 ns | 419.21 us | +inf% |
| Table to/fromJson | 1M | 0 ns | 419.32 us | +inf% |
| Table to/fromJson | 10K | 0 ns | 419.65 us | +inf% |
| Table serial/deserial | 1M | 0 ns | 751.10 us | +inf% |
| Table serial/deserial | 1M | 0 ns | 749.68 us | +inf% |
| Table serial/deserial | 10K | 0 ns | 749.65 us | +inf% |

## Serializer

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| exportTableToFile | 1M | 0 ns | 741.04 us | +inf% |
| exportTableToFile | 1M | 0 ns | 743.04 us | +inf% |
| exportTableToFile | 10K | 0 ns | 765.06 us | +inf% |
| importTableFromFile | 1M | 0 ns | 422.04 us | +inf% |
| importTableFromFile | 1M | 0 ns | 422.84 us | +inf% |
| importTableFromFile | 10K | 0 ns | 420.59 us | +inf% |
| exportDatabaseToJson | 1M | 0 ns | 864.41 us | +inf% |
| exportDatabaseToJson | 1M | 0 ns | 932.79 us | +inf% |
| exportDatabaseToJson | 10K | 0 ns | 875.31 us | +inf% |
| importDatabaseFromJson | 1M | 0 ns | 428.71 us | +inf% |
| importDatabaseFromJson | 1M | 0 ns | 429.14 us | +inf% |
| importDatabaseFromJson | 10K | 0 ns | 430.08 us | +inf% |

## Write Ahead Log

| Test | Iteration | Current | v1.0.0 | Δ |
|---|---|---|---|---|
| wal append | 1M | 0 ns | 285.85 us | +inf% |
| wal append | 1M | 0 ns | 242.21 us | +inf% |
| wal append | 10K | 0 ns | 223.13 us | +inf% |
| wal entryAt | 10K | 0 ns | 1.23 us | +inf% |
| wal entryAt | 100K | 0 ns | 1.23 us | +inf% |
| wal entryAt | 1M | 0 ns | 1.23 us | +inf% |
| wal range(100) | 10K | 0 ns | 126.89 us | +inf% |
| wal range(100) | 100K | 0 ns | 126.88 us | +inf% |
| wal range(100) | 1M | 0 ns | 127.26 us | +inf% |
| wal open(1000-entry scan) | 1M | 0 ns | 1.34 ms | +inf% |
| wal open(1000-entry scan) | 1M | 0 ns | 1.34 ms | +inf% |
| wal open(1000-entry scan) | 10K | 0 ns | 1.34 ms | +inf% |
| wal append+truncateFrom | 1M | 0 ns | 427.70 us | +inf% |
| wal append+truncateFrom | 1M | 0 ns | 438.91 us | +inf% |
| wal append+truncateFrom | 10K | 0 ns | 367.36 us | +inf% |

## Summary

| Result | Count |
|---|---|
| Current faster | 273 (100%) |
| v1.0.0 faster | 0 (0%) |
| Tie | 0 (0%) |
