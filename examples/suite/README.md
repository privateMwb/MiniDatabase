# Examples Suite

This document describes the example categories under `examples/suite/`
— what each one covers, and the individual examples it contains.

| Category | Focus |
|---|---|
| [Quickstart](#quickstart) | The smallest useful program at each layer: one Record, one save, one full CRUD cycle |
| [Patterns](#patterns) | Idiomatic ways to use QueryEngine, schemas, and Status together |
| [Integration](#integration) | Multiple MiniDB pieces used together the way a real application would |
| [Advanced](#advanced) | Compositions and manual workarounds for gaps MiniDB doesn't cover itself |
| [Misuse](#misuse) | Common mistakes, what MiniDB does instead of what you might expect, and the fix |
| [Conventions](#conventions) | Naming pattern and macro gotchas specific to this suite |

---

## Quickstart

The smallest useful program at each layer -- meant to be read top to
bottom before anything else in this suite.

### Examples

| File | What it covers |
|---|---|
| `hello_miniDB.cpp` | Create a `Database`, create a `Table` with a schema, insert one `Record`, read it back |
| `basic_crud.cpp` | `insertRecord`/`getRecord`/`updateRecord`/`deleteRecord` on one `Table`, and that delete is logical until `compact()` |
| `save_and_load.cpp` | Persisting a `Database` to disk with `save()`/`load()`, atomicity, and that a failed load leaves the target untouched |

---

## Patterns

Idiomatic ways to combine `QueryEngine`, schemas, and `Status` -- the
recurring shapes worth reaching for rather than reinventing.

### Examples

| File | What it covers |
|---|---|
| `filtering_and_sorting.cpp` | A single `FilterPredicate`, combining predicates with AND, sorting ascending/descending, and `selectAll()` as "no filter" |
| `pagination.cpp` | Keyset (cursor) pagination via sort + limit + "greater than the last value seen", since `RecordID` itself isn't queryable |
| `schema_design.cpp` | Defining a `Vector<ColumnDef>`, the four `ColumnType` kinds, nullable vs required columns, INT vs DOUBLE |
| `status_handling.cpp` | Checking `[[nodiscard]] Status` instead of exceptions, early-return chaining, and explicit `(void)` discards |

---

## Integration

Multiple MiniDB pieces used together the way a real application would,
rather than one class in isolation.

### Examples

| File | What it covers |
|---|---|
| `full_application_workflow.cpp` | `Database` + `Table` + `QueryEngine` + `Serializer` in one session: build a schema, load data, query it, export a table, persist the database |
| `concurrent_save_load.cpp` | `Concurrency` driving parallel save/load across multiple tables in one `Database`, one file and worker thread per table |
| `storage_engine_buffer_pool.cpp` | `StorageEngine`'s per-page API (`writePage`/`fetchPage`/`evictPage`) used directly against real files, independent of `Table` |

---

## Advanced

Compositions and manual workarounds for gaps MiniDB doesn't close
itself -- multi-call pipelines, hand-rolled compensation, and direct
cache management.

### Examples

| File | What it covers |
|---|---|
| `aggregate_query_pipeline.cpp` | Chaining filter + sort + limit into one `select()`, then running `sum`/`avg`/`max`/`min` as separate calls over the same predicates |
| `multi_table_consistency_by_hand.cpp` | MiniDB has no cross-table transactions; the manual compensating-action pattern, and exactly where it can still fail |
| `custom_buffer_pool_usage.cpp` | Manually managing a `StorageEngine` page's lifecycle: cache, check dirty/cached state, flush, evict, and why flush-before-evict keeps disk in sync |

---

## Misuse

Common mistakes, what MiniDB actually does instead of what you might
expect, and the fix -- each file is a trap first, then the correction.

### Examples

| File | What it covers |
|---|---|
| `duplicate_record_Ids.cpp` | `insertRecord()` fails with `DUPLICATE_KEY` on a reused id rather than overwriting; `updateRecord()` is the fix; the "reused id counter after a delete" trap |
| `evictPage_is_not_a_delete.cpp` | `evictPage()` only drops a page from the cache (flushing first if dirty) -- it never deletes anything from disk |
| `move_only_types.cpp` | `Page`/`Table`/`Database` are move-only; the compile error copying one would produce; passing by reference vs `std::move()` |
| `schema_mismatch_on_import.cpp` | Import functions validate incoming data against the *target* table's existing schema, not the exported data's origin, and a partial import stops at the first bad record |

---

## Conventions

- Every example is a single free function, `run_examples()`, registered
  with `REGISTER_EXAMPLE_SUITE()` at the bottom of the file -- one
  example file, one suite, no shared driver across files.
- `setTitle("...")` marks off each numbered step within a file's
  console output; it's cosmetic only and has no effect on control flow.
- Examples print status codes as `static_cast<int>(s)`, with the
  expected value spelled out in a trailing comment (e.g.
  `// (DUPLICATE_KEY)`) rather than asserted -- these are narrated
  demonstrations to read and run, not `CHK()`-style test cases.
- `(void)` prefixes a call whose `[[nodiscard]] Status` is deliberately
  ignored, consistent with `status_handling.cpp`'s own guidance on when
  that's appropriate.
- Examples that touch disk create their own scratch directory (or a
  file under `std::filesystem::temp_directory_path()`) and remove it at
  the end of `run_examples()`, so running the suite repeatedly never
  leaves stale files behind.
