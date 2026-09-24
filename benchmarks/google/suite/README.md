# Google Benchmark Suite

This document describes the benchmark categories under
`benchmarks/google/suite/` — what each one measures, and the individual
benchmarks it contains.

| Category | Focus |
|---|---|
| [Access](#access) | Read and lookup operations against an already-built page, table, cache, or query |
| [Core](#core) | Frequently-called record/page/table mutators and table-management operations |
| [Lifecycle](#lifecycle) | Construction, copy, and move of the Core value types |
| [Scaling](#scaling) | Per-operation cost as table count, page count, or record count grows |
| [Utility](#utility) | Storage, serialization, and concurrency primitives underneath Core |

There's no natural "std" equivalent to benchmark MiniDatabase against —
no standard-library embedded database exists to pair each benchmark
with. Every `BENCHMARK()` registration below times MiniDatabase alone.

Every benchmark below runs inside a `benchmark::State` loop, which
Google Benchmark repeats and calibrates automatically to produce a
stable measurement, so there's no manual iteration-count tiering to
configure. The **Scaling** category below measures something
different: how per-operation cost changes as some structural size
(table count, page count, record count) grows, independent of how many
times Google Benchmark itself repeats the call.

---

## Access

Benchmarks read and lookup operations against an already-built page,
table, cache, or query — the cost of finding something that's already
there (or confirming it isn't), not the cost of putting it there.

### Benchmarks

| File | What it covers |
|---|---|
| `page_access.cpp` | `Page::getRecord()` (id-based scan, hit/miss) vs `getRecordAt()` (index-based, hit/miss) |
| `table_access.cpp` | `Table::getRecord()` through the id→page index, on a single-page and a multi-page table, plus a miss |
| `storage_engine_cache.cpp` | `StorageEngine::fetchPage()` cache hit vs miss (disk read), and `getCachedPage()` |
| `query_engine_select.cpp` | `QueryEngine::selectAll()`, `select()` with an equality predicate, and `select()` with a sort |

---

## Core

Benchmarks the fundamental, most frequently exercised mutators — adding,
changing, and removing records and fields, and managing a database's set
of tables.

### Benchmarks

| File | What it covers |
|---|---|
| `record_field.cpp` | `Record::setField()`, `getField()`, `getFieldRef()`, `hasField()`, `removeField()` |
| `page_mutation.cpp` | `Page::addRecord()`, `updateRecord()`, `deleteRecord()`, `compact()` |
| `table_mutation.cpp` | `Table::insertRecord()`, `updateRecord()`, `deleteRecord()`, `rebuildIndex()` |
| `database_table_ops.cpp` | `Database::createTable()`, `dropTable()`, `getTable()`, `hasTable()` |

---

## Lifecycle

Benchmarks object lifetime operations — construction, copy, and move —
across the Core value types. `Page`, `Table`, and `Database` are
move-only; `Record` is copyable, so its suite contrasts copy against
move directly.

### Benchmarks

| File | What it covers |
|---|---|
| `record.cpp` | `Record` default/id/id+data construction, copy construction, move construction |
| `page.cpp` | `Page` default/id construction, move construction (empty and full) |
| `table.cpp` | `Table` construction (empty and 20-column schema), move construction (empty and populated) |
| `database.cpp` | `Database` construction, move construction (empty and 50-table) |

---

## Scaling

Benchmarks how per-operation cost changes as a structural size grows —
table count, page count, or record count — as opposed to Access/Core's
fixed-size snapshots. Directly contrasts index-based operations that
should stay flat (`getRecord()`, `getTable()`) against scan-based ones
that shouldn't (`insertRecord()`'s `findPageWithSlot()`, `select()`'s
full scan and sort).

### Benchmarks

| File | What it covers |
|---|---|
| `table_scaling.cpp` | `Table::insertRecord()` and `getRecord()` cost at 1, 100, and 1000 existing pages |
| `database_scaling.cpp` | `Database::createTable()` and `getTable()` cost at 10, 100, and 200 existing tables |
| `queryEngine.cpp` | `QueryEngine::select()` with a predicate, and with a sort, at 1, 100, and 1000 pages |
| `concurrency_scaling.cpp` | `Concurrency::rebuildAllIndexesParallel()` cost at table counts below, at, above, and well above the thread pool size |

---

## Utility

Benchmarks the storage, serialization, and concurrency primitives that
Core sits on top of — not called directly by most application code, but
underneath nearly every operation above.

### Benchmarks

| File | What it covers |
|---|---|
| `file_iO.cpp` | `FileIO::writeSlot()`/`readSlot()` and `writeFileAtomic()`/`readFile()` round trips |
| `write_ahead_log.cpp` | `WriteAheadLog::append()`, `entryAt()`/`range()`, `open()`'s recovery scan, `truncateFrom()` |
| `json_round_trip.cpp` | `Record`/`Page`/`Table` tree-based (`toJson`/`fromJson`) vs string-based (`serialize`/`deserialize`) round trips |
| `serializer.cpp` | `Serializer::exportTableToFile()`/`importTableFromFile()` and `exportDatabaseToJson()`/`importDatabaseFromJson()` round trips |
| `concurrency.cpp` | `Concurrency::saveAllTablesParallel()`, `loadAllTablesParallel()`, `rebuildAllIndexesParallel()`, `exportAllTablesParallel()` |

---

## Conventions

- **Solo case only** — every benchmark is a single free function,
  `bench_<name>`, registered with its own `BENCHMARK(...)` call. There is
  no MiniDatabase/std pairing: no standard-library embedded database
  exists to pair against.
- **Growth tiers** — Scaling-category files register one `BENCHMARK()`
  per fixed structural size (`bench_get_10`, `bench_get_100`,
  `bench_get_200`, ...) rather than using Google Benchmark's
  `Args()`/`Range()` facility, so each tier's size stays visible directly
  in its name and in `--benchmark_filter` output.
- All benchmarks build into a single binary and run through a shared
  `BENCHMARK_MAIN()` entry point defined once outside these files — no
  per-file `BENCHMARK_MAIN()` and no custom suite registration.
- `benchmark::DoNotOptimize(...)` is used in place of the old
  `(void)result;` discards. For anything larger than a register (strings,
  `Record`, `QueryResult`), passing the object itself is fine for
  read-only results, but prefer a pointer (`&r`, `.data()`) when
  benchmarking a buffer that's mutated in place.
- Setup that shouldn't be timed (seeding a `Table`/`Database`, writing a
  file to disk, opening a `WriteAheadLog`) happens before the
  `for (auto _ : state)` loop, mirroring the custom suite's convention of
  keeping fixture construction outside the timed lambda.
- Use `--benchmark_filter=<regex>` to run a subset, and
  `--benchmark_out=<file> --benchmark_out_format=json` to capture results
  for comparison across runs (e.g. with `compare.py` from the Google
  Benchmark tooling).
