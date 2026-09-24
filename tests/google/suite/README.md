# Google Test Suite

This document describes the test categories under `tests/google/suite/`
— what each one covers, and the individual tests it contains.

| Category | Focus |
|---|---|
| [Unit](#unit) | A single class in isolation: Record, Page, Table, Database, FileIO, QueryEngine, WriteAheadLog |
| [Lifecycle](#lifecycle) | Cross-cutting contracts every type must honor: move-only ownership, id monotonicity, serialization-path equivalence, status reporting |
| [Integration](#integration) | Multiple classes exercised together the way a real session would: persistence, per-page I/O, a full mutation sequence |
| [Concurrency](#concurrency) | The parallel batch operations: save/load/rebuild/export across many tables, and the fold semantics when some of them fail |
| [Regression](#regression) | One test file per fixed bug, pinning the specific behavior that broke |

---

## Unit

Verifies a single class in isolation — no other MiniDB class involved
unless the class under test depends on it directly (e.g. Table on Page).

### Tests

| File | What it covers |
|---|---|
| `record.cpp` | Field access, `validate()` against a schema, `toJson`/`fromJson` and `serialize`/`deserialize` round trips, the deleted flag |
| `page.cpp` | Construction, `addRecord`/`getRecord`/`getRecordAt`/`updateRecord`/`deleteRecord`, `compact()`, serialization round trips, move construction/assignment |
| `table.cpp` | CRUD, schema introspection, page allocation across the `MAX_RECORDS_PAGE` boundary, `compact()`/`rebuildIndex()`, serialization round trips |
| `database.cpp` | `createTable`/`dropTable`/`getTable`/`hasTable`, `toJson`/`fromJson`, `save`/`load` round trips, a corrupt-file load, `compact()` |
| `fileIO.cpp` | `writeFileAtomic`/`readFile` and `writeSlot`/`readSlot` round trips, an oversized slot payload, a corrupt length prefix |
| `query_engine.cpp` | `selectAll`/`selectByID`, single and combined predicates, `limit`, ASC/DESC sort, `count`/`sum`/`avg`/`max`/`min` |
| `write_ahead_log.cpp` | `open`/`append`/`entryAt`/`range`/`truncateFrom`, crash recovery from a truncated or checksum-mismatched tail, empty-file and single-entry edges |

---

## Lifecycle

Verifies contracts that apply across every serializable/movable type
rather than to one class's specific behavior.

### Tests

| File | What it covers |
|---|---|
| `move_only.cpp` | Page/Table/Database are copy-disabled and move-valid, checked at compile time and at runtime |
| `id_monotonicity.cpp` | PageID/TableID never get reassigned once handed out, across many allocations, reload cycles, and create/drop cycles |
| `serialization_roundtrip.cpp` | `toJson`/`fromJson` and `serialize`/`deserialize` reconstruct identical state, across Record/Page/Table/Database |
| `status_handling.cpp` | Malformed input to a `deserialize`/`fromJson` boundary reports `Status::PARSE_ERROR`; StorageEngine never drops a dirty page without persisting or reporting failure |

---

## Integration

Verifies multiple MiniDB classes used together the way a real
application or session would, rather than one class in isolation.

### Tests

| File | What it covers |
|---|---|
| `table_database_lifecycle.cpp` | A full session: multi-page insert/update/delete, dropping and recreating a table mid-session, `compact()`, then a save/load round trip |
| `database_save_load.cpp` | Multi-table, multi-page save/load through a real file, reloaded data queried through QueryEngine, and a reload-mutate-resave cycle |
| `serializer.cpp` | Serializer's table- and database-level export/import against real files — a different on-disk shape than `Database::save`/`load` |
| `storage_engine_per_page_io.cpp` | `writePage`/`readPageFromDisk`/`fetchPage`/`cachePage`/`evictPage` together across multiple pages and tables, including a cold-start (fresh instance, empty cache) read |

---

## Concurrency

Verifies the parallel batch operations that drive save/load/rebuild/
export across every table in a Database at once, including what happens
when some of those tables fail and others don't.

### Tests

| File | What it covers |
|---|---|
| `save_load.cpp` | `saveAllTablesParallel`/`loadAllTablesParallel` round trip, correctness at a table count exceeding the thread pool size, a duplicate-key load |
| `export.cpp` | `exportAllTablesParallel` writing one file per table, soft-deleted records excluded, scale beyond the thread pool size, an empty table |
| `rebuild_index.cpp` | `rebuildAllIndexesParallel` preserves every table's lookups, an empty database, scale beyond the thread pool size, tables with zero records |
| `error_propagation.cpp` | The fold picks the *last* non-OK result in table-creation order, not the first or the worst; one table's failure doesn't roll back another's success |

---

## Regression

One file per fixed bug. Each pins the specific behavior that broke,
named after the bug rather than the feature.

### Tests

| File | What it covers |
|---|---|
| `table_id_collision.cpp` | Page ids were sourced from `pages.size()` instead of a monotonic counter resumed from the highest loaded id |
| `database_id_collision.cpp` | Table ids had the same `tables.size()` bug, which also collided after `dropTable()` shrank the container |
| `database_atomic_write.cpp` | `save()`'s write-to-`.tmp`-then-rename guarantee: a blocked write leaves the existing file untouched, and recovers once the obstruction clears |
| `database_load_atomicity.cpp` | `fromJson`'s scratch-then-swap: a mid-load failure leaves name/tables and the id counter completely untouched |
| `record_validate_zero.cpp` | INT-column validation used a truthiness check that rejected the legitimate value `0` |
| `query_engine_sort_off_by_one.cpp` | The hand-rolled bubble sort's inner-loop bound read one element past the end of the array |
| `storage_engine_evict_dirty.cpp` | `evictPage()` used to erase a cache entry unconditionally, silently discarding an unflushed dirty page |
| `table_copy_assign_signature.cpp` | A signature typo (`const Table&&` instead of `const Table&`) failed to suppress the compiler-generated copy-assignment operator |

---

## Conventions

- Every test is a `TEST(SuiteName, CaseName)` — there's no `run_tests()`
  and no `REGISTER_TEST_SUITE()`; GoogleTest discovers every `TEST()` in
  the binary automatically.
- `ASSERT_*` is used wherever a later line in the same case depends on
  the check succeeding (a lookup before reading its result, a status
  check before touching the value it guards); `EXPECT_*` is used for
  independent checks, so a case reports every mismatch it finds instead
  of stopping at the first.
- Compile-time contract checks (`move_only.cpp`,
  `table_copy_assign_signature.cpp`) still use `static_assert` directly,
  unchanged by the framework switch — a regression there fails to
  **compile**, not just fails a runtime check.
- `table_copy_assign_signature.cpp`'s placeholder `CHK(true)` (the real
  assertions are the `static_assert`s above it) became `SUCCEED()`.
- Build target links against `gtest`/`gtest_main` (or a shared `main()`
  translation unit), not the repo's own test-support library.
