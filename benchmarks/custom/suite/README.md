# Benchmark Suite

This document describes the benchmark categories under `benchmarks/custom/suite/`
— what each one measures, and the individual benchmarks it contains.

| Category | Focus |
|---|---|
| [Access](#access) | Read and lookup operations against an already-built page, table, cache, or query |
| [Core](#core) | Frequently-called record/page/table mutators and table-management operations |
| [Lifecycle](#lifecycle) | Construction, copy, and move of the Core value types |
| [Scaling](#scaling) | Per-operation cost as table count, page count, or record count grows |
| [Utility](#utility) | Storage, serialization, and concurrency primitives underneath Core |
| [Conventions](#conventions) | Naming pattern and macro gotchas specific to this suite |

Unlike FalconHTTP's suite, this one runs on real Google Benchmark rather
than the repo's custom `<support/framework.h>` — see [Conventions](#conventions)
for what changed in the port and why. There's no natural "std" equivalent
to benchmark MiniDatabase against either — no standard-library embedded
database exists to pair each benchmark with — so every registered
benchmark times MiniDatabase alone.

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

This suite was ported from the repo's original `<support/framework.h>`
harness (`BENCH_SOLO()`/`BENCH_SOLO_CUSTOM()` + `REGISTER_BENCH_SUITE()`)
to real Google Benchmark. A few things changed in the port:

- **Include**: every file uses `<benchmark/benchmark.h>` plus
  `<MiniDB/MiniDatabase.h>` (or the relevant sub-header, e.g.
  `<MiniDB/Storage/WriteAheadLog.h>`), not `<support/framework.h>`.
- **Registration**: each case is a free `static void` function taking
  `benchmark::State&`, timing its operation inside `for (auto _ : state)`,
  registered with `BENCHMARK(fn)->Name("Human Readable Name")` — there's
  no `run_benchmarks()` driver or `REGISTER_BENCH_SUITE()` call; Google
  Benchmark discovers every `BENCHMARK()` registration automatically.
- **One function per case**: the original framework let one function
  run several named `BENCH_SOLO()`/`BENCH_SOLO_CUSTOM()` calls back to
  back (e.g. three table sizes, or a tree-vs-string pair) with
  `std::cout << "\n"` separating them. Google Benchmark needs one
  function per registration, so every grouped case here was split into
  individual functions — e.g. `bench_get_at_scale()` became
  `bench_get_10`/`bench_get_100`/`bench_get_200`, each with its own
  `BENCHMARK()` line.
- **No manual iteration tiers**: `BENCH_SOLO()` repeated each case at
  fixed SMALL/MEDIUM/LARGE counts. Google Benchmark's `state` loop
  calibrates its own iteration count per case, so there's no equivalent
  tiering here — apply `--benchmark_min_time` at the command line
  instead if a specific tier's worth of runtime is needed.
- **`doNotOptimize` → `benchmark::DoNotOptimize`**: same purpose
  (prevent the compiler from eliding a computed-but-unused result),
  different spelling.
- **`BENCH_SOLO` vs `BENCH_SOLO_CUSTOM` distinction dropped**: the
  original macro choice flagged fsync-heavy/expensive operations for
  different default iteration counts. Google Benchmark calibrates
  automatically regardless of per-call cost, so both macros map onto
  the same `BENCHMARK()` registration here.
