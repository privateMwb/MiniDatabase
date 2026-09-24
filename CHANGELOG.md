# Changelog

All notable changes to MiniDatabase are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Nothing yet.

## [1.0.0] - 2026-08-03

The first stable release of MiniDatabase, an embedded, in-process database
engine for modern C++23.

### Added
- In-process, embedded storage engine — no server, no connection, no
  network protocol.
- Schema-validated records: `STRING`, `INT`, `DOUBLE`, and `BOOL` columns,
  each nullable or required, with a real integral check for `INT`.
- Full CRUD on tables: `insertRecord()`, `getRecord()`, `updateRecord()`,
  and `deleteRecord()`, reporting failure through `Status` rather than
  exceptions.
- Fixed-slot page storage: every page occupies exactly `PAGE_SIZE` bytes
  at a fixed `id * PAGE_SIZE` disk offset, giving O(1) random-access I/O
  by page id.
- `StorageEngine`: LRU-backed page cache (buffer pool) with explicit
  `fetchPage()`, `flushPage()`, and `evictPage()` control; a dirty page is
  always flushed before eviction.
- Atomic whole-database persistence: `save()` writes to a temp file and
  renames over the destination, so a crash mid-write never corrupts the
  existing file.
- All-or-nothing `load()`: a corrupt or truncated file can't leave the
  database half-replaced.
- `QueryEngine`: AND-combined predicate filtering, sorting, result
  limiting, and `count`/`sum`/`avg`/`max`/`min` aggregates.
- `Serializer`: table- and database-level JSON export/import, independent
  of the primary save/load path.
- `Concurrency`: thread-pool-backed parallel save, load, index rebuild,
  and export across every table in a database, reporting one table's
  failure without rolling back another table's success.
- O(1) `PageID -> Page*` and `TableID -> Table*` hash indexes.
- Move-only `Page`, `Table`, and `Database`, so storage ownership can't be
  silently duplicated by an accidental copy.
- CMake package (`find_package(MiniDB)` / `MiniDB::MiniDB`) plus Conan
  recipe and vcpkg port.
- Built directly on `VectorPro`, `JsonPro`, `PoolPro`, `HashMapPro`,
  `CachePro`, `ArenaPro`, and `ThreadPoolPro`.

### Performance
- Fixed `PAGE_SIZE` slots give every page a true disk offset — no directory
  scan to locate a page.
- The LRU page cache avoids re-reading and re-parsing hot pages from disk.
- Pool-allocated records within a page avoid per-record heap churn on
  insert.
- `toJson()`/`fromJson()` build and consume the in-memory tree directly,
  with no redundant dump-then-reparse round trip at each nesting level
  (record, page, table, database).
- Atomic saves cost one `fsync` and one `rename`, not one per record.
- Parallel save/load/rebuildIndex/export share a single thread pool across
  every table in a database.
- Benchmarked as absolute measurements, since there is no `std::` embedded
  database to compare against. At 1M iterations a `fetchPage()` cache hit
  is roughly 100x faster than a miss (53.92 ms vs. 5.22 s), and
  `Table::getRecord()` is faster on a 1000-page table (22.28 ms) than on a
  single-page table (72.69 ms), since page count is no longer a cost
  variable in the lookup path. A sorted `QueryEngine` select is roughly
  17x its unsorted equivalent at 100 iterations (7.30 s vs. 409.38 ms).
  Full results in `benchmarks/baselines/v1.0.0.md`.

### Testing
- Comprehensive test suite covering unit-level correctness for `Record`,
  `Page`, `Table`, `Database`, `QueryEngine`, and `FileIO`; cross-component
  integration (full save/load round trips, `Serializer` export/import,
  `StorageEngine` per-page I/O against real files, multi-table and
  multi-page lifecycles); regression tests pinning every bug fixed during
  development; contract-level invariants (id monotonicity, move-only
  ownership, serialization-path equivalence, consistent `Status`
  reporting); and concurrency correctness under the shared thread pool,
  including error propagation and partial failure.
- 90.4% line coverage (849/939 lines) and 90.8% function coverage
  (139/153 functions), excluding test infrastructure and submodule
  dependencies.

### CI
- Automated builds and tests across GCC, Clang, MSVC, and AppleClang, each
  in Debug and Release configurations.
- ASan/UBSan and a separate TSan job, the latter exercising
  `Concurrency`'s real thread-pool usage.
- `clang-format` and `clang-tidy` checks.
- Code coverage collection.
- Conan and vcpkg packaging, each verified with a consumer smoke test
  against the built package.
- Automatic update of the Conan recipe, vcpkg port, and version metadata
  when a `v*` tag is pushed.

[Unreleased]: https://github.com/privateMwb/MiniDatabase/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/privateMwb/MiniDatabase/releases/tag/v1.0.0
