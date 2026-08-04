# MiniDB v1.0.0

The first stable release of **MiniDB**, an embedded C++ database engine built on this project's own allocator, container, concurrency, and JSON libraries.

## Highlights

- In-process, embedded storage engine — no server, no connection, no network protocol.
- Schema-validated records: `STRING`, `INT`, `DOUBLE`, and `BOOL` columns, each nullable or required.
- Full CRUD on tables: `insertRecord()`, `getRecord()`, `updateRecord()`, `deleteRecord()`.
- Fixed-slot page storage with true O(1) random-access disk I/O by page id.
- LRU-backed page cache (buffer pool) with explicit `fetchPage()` / `flushPage()` / `evictPage()` control.
- Atomic whole-database persistence: `save()`/`load()` via temp-file-plus-rename, never a partial write.
- All-or-nothing load semantics — a corrupt or truncated file can't leave the database half-replaced.
- `QueryEngine`: predicate filtering, sorting, result limiting, and `count`/`sum`/`avg`/`max`/`min` aggregates.
- `Serializer`: table- and database-level JSON export/import, independent of the primary save/load path.
- `Concurrency`: thread-pool-backed parallel save, load, index rebuild, and export across every table in a database.
- Move-only `Page`/`Table`/`Database` — storage ownership can't be silently duplicated by an accidental copy.
- Built directly on `VectorPro`, `JsonPro`, `PoolPro`, `HashMapPro`, `CachePro`, `ArenaPro`, and `ThreadPoolPro`.

## Performance

The storage and query paths are designed around avoiding unnecessary I/O, allocation, and scanning:

- Fixed `PAGE_SIZE` slots give every page a true `id * PAGE_SIZE` disk offset — no directory scan to locate a page.
- The LRU page cache avoids re-reading and re-parsing hot pages from disk.
- Pool-allocated records within a page avoid per-record heap churn on insert.
- `O(1)` `PageID → Page*` and `TableID → Table*` indexes replace what were originally linear scans.
- `toJson()`/`fromJson()` build and consume the in-memory tree directly — no redundant dump-then-reparse round trip at each nesting level (record → page → table → database).
- Atomic saves cost one `fsync` and one `rename`, not one per record.
- Parallel save/load/rebuildIndex/export share a single thread pool across every table in a database.

## Benchmarks

Unlike `CachePro`, MiniDB has no natural drop-in standard-library
equivalent to benchmark against (there's no `std::` embedded database) --
these are absolute measurements, not a comparison. Figures below are at
1M iterations; full results across every subsystem and scale:
`benchmarks/results/v1_0_0.md`.

| Test                                      | 1M       |
|--------------------------------------------|----------|
| Table GetRecord (single page)               | 72.69 ms |
| Table GetRecord (multi page, 1000 pages)    | 22.28 ms |
| StorageEngine FetchPage (cache hit)         | 53.92 ms |
| StorageEngine FetchPage (cache miss)        | 5.22 s   |
| Database GetTable (200 tables)              | 21.16 ms |
| FileIO WriteFileAtomic (100 iterations)     | 43.11 ms |
| Concurrency SaveAllTablesParallel (100 iterations) | 169.05 ms |

A few things worth calling out honestly:

- **The page cache is doing real work**: a `FetchPage` hit is roughly
  **~100x faster** than a miss at 1M (53.92 ms vs 5.22 s). That gap is
  the entire argument for the buffer pool existing.
- **`Table GetRecord` is actually *faster* per lookup when a table spans
  more pages** (22.28 ms for a 1000-page table vs 72.69 ms for a
  single-page table, both at 1M) -- this looks counterintuitive but
  reflects the O(1) `pageIndex` lookup doing its job: page count stops
  being a cost variable in the lookup path at all.
- **`QueryEngine`'s sorted select is the single most expensive operation
  measured**, and by a wide margin -- `Select Sorted (1000 pages)` at 100
  iterations takes 7.30 s versus 409.38 ms for the equivalent unsorted
  `Select Eq` at the same scale, roughly a 17x gap. Expected, given
  `std::sort`'s O(n log n) versus a linear predicate scan -- but worth
  knowing explicitly before running a sort over a large result set on a
  latency-sensitive path.

## Testing

The project includes a comprehensive test suite covering:

- Unit-level correctness for `Record`, `Page`, `Table`, `Database`, `QueryEngine`, and `FileIO` in isolation
- Cross-component integration: full save/load round trips, `Serializer` export/import, `StorageEngine` per-page I/O against real files, and multi-table/multi-page lifecycle scenarios
- Regression coverage pinning every bug fixed during development (sort correctness, id-collision safety, schema validation edge cases, atomic-write and load-atomicity guarantees, dirty-page eviction safety, and more) so none of them can silently return
- Contract-level invariants: id monotonicity, move-only ownership, serialization-path equivalence, and consistent `Status` reporting with no silent data loss
- Concurrency correctness: parallel save/load/export/rebuildIndex under the shared thread pool, including error-propagation and partial-failure behavior

## Code Coverage

The current test suite achieves:

- **90.4% line coverage** (849 / 939 lines)
- **90.8% function coverage** (139 / 153 functions)

| Directory | Line Coverage | Function Coverage |
|-----------|---------------|--------------------|
| `Core`    | 92.1% (481/522) | 94.5% (86/91) |
| `Engine`  | 89.6% (294/328) | 84.5% (49/58) |
| `Common`  | 83.1% (74/89)   | 100.0% (4/4)  |

Coverage reports exclude test infrastructure and submodule dependencies
(`libs/internal/`), focusing on MiniDB's own implementation.
`Engine`'s function coverage is the softest spot of the three -- worth a
look before the next release to see which functions there aren't
exercised yet.

## Continuous Integration

Automated builds, tests, and checks are configured across:

- Multi-compiler builds (GCC, Clang, and MSVC configurations)
- ASan/UBSan and a separate TSan job — the latter specifically exercising `Concurrency`'s real thread-pool usage
- `clang-format` and `clang-tidy` checks on every push and pull request
- Code coverage collection, with submodule and test-infrastructure sources excluded from the measured result
- Conan and vcpkg packaging, each verified with a real consumer smoke test against the built package
- A weekly submodule-update workflow that bumps all 7 internal dependencies, builds, and runs the full test suite before ever proposing the change — broken bumps are caught before merge, not after

*(Exact compiler/configuration matrix should be double-checked against `build.yml`'s current contents before publishing — noting here so it isn't asserted without confirmation.)*

## Release

This release represents the first stable version of MiniDB and establishes its initial storage engine, query engine, concurrency model, and cross-platform build and packaging pipeline.

## Installation

Clone the repository and integrate MiniDB into your C++ project using the provided CMake configuration.

See the project documentation for build instructions, API usage, and integration details.
