# MiniDatabase

<p align="center">
  <img src="https://img.shields.io/github/v/release/privateMwb/MiniDatabase?style=for-the-badge&logo=github&color=yellow" alt="Version">
  <img src="https://img.shields.io/badge/License-MIT-orange?style=for-the-badge" alt="License - MIT">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue?style=for-the-badge&logo=c%2B%2B" alt="C++ - 23">
</p>

<p align="center">
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/build.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/build.yml/badge.svg" alt="Build and Test">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/benchmark.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/benchmark.yml/badge.svg" alt="Benchmarks">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/coverage.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/coverage.yml/badge.svg" alt="Coverage">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/sanitizers.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/sanitizers.yml/badge.svg" alt="Sanitizers">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/clang-tidy.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/clang-tidy.yml/badge.svg" alt="Clang Tidy">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/clang-format.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/clang-format.yml/badge.svg" alt="Clang Format">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/docs.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/docs.yml/badge.svg" alt="Documentation">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/release.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/release.yml/badge.svg" alt="Release">
  </a>
  <a href="https://github.com/privateMwb/MiniDatabase/actions/workflows/packaging.yml">
    <img src="https://github.com/privateMwb/MiniDatabase/actions/workflows/packaging.yml/badge.svg" alt="Packaging">
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/GCC-support-B46F1B?style=flat&logo=gnu" alt="GCC - support">
  <img src="https://img.shields.io/badge/Clang-support-045891?style=flat&logo=llvm" alt="Clang - support">
  <img src="https://img.shields.io/badge/MSVC-support-5C2D91?style=flat" alt="MSVC - support">
  <img src="https://img.shields.io/badge/AppleClang-support-000000?style=flat&logo=apple" alt="AppleClang - support">
</p>

MiniDB is an embedded, in-process C++ database engine — no server, no connection, no network protocol. Schema-validated records, a fixed-slot page storage engine with an LRU-backed buffer pool, atomic whole-database persistence with all-or-nothing load semantics, and a thread-pool-backed concurrent save/load/export path, built entirely on this author's own allocator, container, concurrency, and JSON libraries.

## 📑 Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Dependencies](#dependencies)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Development](#development)
- [Benchmarks](#benchmarks)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Changelog](#changelog)
- [License](#license)

## <a id="features"></a>✨ Features

- **Fixed-slot page storage** — every page occupies exactly `PAGE_SIZE` bytes at a fixed `id * PAGE_SIZE` disk offset, so locating a page is a direct calculation, never a directory scan.
- **LRU-backed buffer pool** — `StorageEngine` caches hot pages in memory (`fetchPage()`/`flushPage()`/`evictPage()`), and a dirty page is always flushed before eviction, never silently discarded.
- **Schema-validated records** — every `Record` is checked against its `Table`'s column types (`STRING`, `INT`, `DOUBLE`, `BOOL`) and nullability before it's ever stored, with a real integral check for `INT` rather than a truthiness shortcut that would reject a legitimate `0`.
- **Atomic, all-or-nothing persistence** — `save()` writes to a sibling temp file and renames over the destination, so a crash mid-write never corrupts the existing file; `load()` parses into scratch state and only swaps it in once every table succeeds, so a corrupt or truncated file can't leave the database half-replaced.
- **O(1) id-indexed lookups** — `PageID → Page*` and `TableID → Table*` both resolve through a hash index, not a linear scan over every page or table.
- **Thread-pool-backed parallel batch operations** — `Concurrency` drives `saveAllTablesParallel()`, `loadAllTablesParallel()`, `rebuildAllIndexesParallel()`, and `exportAllTablesParallel()` across every table in a database on a shared pool, with one table's failure reported without rolling back another table's success.
- **A real query engine** — `QueryEngine` supports predicate filtering (AND-combined via `std::span<const FilterPredicate>`), sorting, result limiting, and `count`/`sum`/`avg`/`max`/`min` aggregates, all operating directly over `Table`'s pages.

## <a id="requirements"></a>📋 Requirements

- A C++23-conformant compiler (tested: GCC, Clang, MSVC, AppleClang)
- CMake 3.20+
- Git submodules initialized — unlike this author's other, dependency-free libraries, MiniDB is a consumer of 7 of them (see [Dependencies](#dependencies)) and needs their source present to build from source

## <a id="dependencies"></a>🔗 Dependencies

MiniDB is built entirely on this author's own libraries, vendored as git submodules under `libs/internal/`:

| Library | Provides | Repository |
|---|---|---|
| VectorPro | `Vector<T>`, used throughout `Table`/`Database` for schema and page storage | [privateMwb/VectorPro](https://github.com/privateMwb/VectorPro) |
| JsonPro | `Json`/`JsonObject`, the serialization format for every `toJson()`/`fromJson()` | [privateMwb/JsonParser](https://github.com/privateMwb/JsonParser) |
| PoolPro | `Pool<>`, backing each `Page`'s record storage | [privateMwb/PoolAllocator](https://github.com/privateMwb/PoolAllocator) |
| HashMapPro | `HashMap<K,V>`, backing every `RecordID`/`PageID`/`TableID` index | [privateMwb/HashMapPro](https://github.com/privateMwb/HashMapPro) |
| CachePro | `LRUCache<K,V>`, backing `StorageEngine`'s page buffer pool | [privateMwb/LRUCache](https://github.com/privateMwb/LRUCache) |
| ArenaPro | `Arena<>`, scoped allocation for `QueryEngine` query execution | [privateMwb/ArenaAllocator](https://github.com/privateMwb/ArenaAllocator) |
| ThreadPoolPro | `ThreadPool`, driving every parallel operation in `Concurrency` | [privateMwb/ThreadPoolPro](https://github.com/privateMwb/ThreadPoolPro) |

## <a id="installation"></a>📦 Installation

**From source:**

```bash
git clone --recurse-submodules https://github.com/privateMwb/MiniDatabase.git
cd MiniDatabase
cmake -B build \
  -DBUILD_TESTS=OFF \
  -DBUILD_BENCHMARKS=OFF \
  -DBUILD_REGRESSION=OFF \
  -DBUILD_EXAMPLES=OFF
cmake --install build
```

Then, in your own `CMakeLists.txt`:

```cmake
find_package(MiniDB CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE MiniDB::MiniDB)
```

> vcpkg and Conan packages are built and verified (recipe in
> `packaging/recipes/minidb/`, port in `packaging/vcpkg/ports/minidb/`),
> but not yet published to the public registries. This section will be
> updated once they are.

## <a id="quick-start"></a>🚀 Quick Start

```cpp
#include <MiniDB/Core/Database.h>
#include <MiniDB/Core/Table.h>
#include <MiniDB/Core/Record.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

int main() {
    Database db("app");
    db.createTable("users", Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"age",  ColumnType::INT,    true},
    });

    Table* users = db.getTable("users");

    Record ada(1);
    ada.setField("name", Json("Ada"));
    ada.setField("age",  Json(30));
    users->insertRecord(ada);

    Record out;
    if (users->getRecord(1, out) == Status::OK) {
        // out.getField("name").asString() == "Ada"
    }
}
```

Filtering, sorting, and aggregating with `QueryEngine`:

```cpp
#include <MiniDB/Engine/QueryEngine.h>

Arena<> arena(DBConstants::ARENA_SIZE);
QueryEngine qe(arena);

FilterPredicate adults{"age", Op::GTE, Json(18)};
SortCondition byAge{"age", SortOrder::ASC};

QueryResult result = qe.select(*users, std::span<const FilterPredicate>(&adults, 1), &byAge);
double averageAge = qe.avg(*users, "age", std::span<const FilterPredicate>(&adults, 1));
```

MiniDB reports failure through `Status`, not exceptions — every mutating call is `[[nodiscard]]` and meant to be checked, not wrapped in a `try`/`catch`:

```cpp
Record duplicate(1);   // id 1 already exists
duplicate.setField("name", Json("Someone Else"));

Status s = users->insertRecord(duplicate);
if (s != Status::OK) {
    // s == Status::DUPLICATE_KEY -- the original record at id 1 is untouched
}
```

## <a id="project-structure"></a>🗂️ Project Structure

```
MiniDatabase/
├── include/
│   └── MiniDB/
│       ├── Common/
│       │   ├── Type.h
│       │   └── FileIO.h
│       ├── Core/
│       │   ├── Record.h
│       │   ├── Page.h
│       │   ├── Table.h
│       │   └── Database.h
│       └── Engine/
│           ├── QueryEngine.h
│           ├── Concurrency.h
│           ├── Serializer.h
│           └── StorageEngine.h
│
├── src/
│   └── MiniDB/
│       ├── Core/
│       │   ├── Record.cpp
│       │   ├── Page.cpp
│       │   ├── Table.cpp
│       │   └── Database.cpp
│       └── Engine/
│           ├── QueryEngine.cpp
│           ├── Concurrency.cpp
│           ├── Serializer.cpp
│           └── StorageEngine.cpp
│
├── libs/
│   └── internal/
│       ├── VectorPro/
│       ├── JsonParser/
│       ├── PoolAllocator/
│       ├── HashMap/
│       ├── LRUCache/
│       ├── ArenaAllocator/
│       └── ThreadPoolPro/
│
├── tests/
│   ├── support/
│   ├── suite/
│   ├── test_main.cpp
│   └── CMakeLists.txt
│
├── benchmarks/
│   ├── support/
│   ├── suite/
│   ├── baselines/
│   ├── bench_main.cpp
│   └── CMakeLists.txt
│
├── examples/
│   ├── support/
│   ├── suite/
│   ├── example_main.cpp
│   └── CMakeLists.txt
│
├── regression/
│   ├── support/
│   ├── regression_main.cpp
│   └── CMakeLists.txt
│
├── packaging/
│   ├── README.md
│   ├── recipes/
│   │   └── minidb/
│   ├── vcpkg/
│   │   └── ports/
│   │       └── minidb/
│   └── vcpkg-smoke-test/
│
├── scripts/
│   └── update_package_files.py
│
├── .github/
│   ├── releases/
│   └── workflows/
│
├── cmake/
│   └── MiniDBConfig.cmake.in
│
├── docs/
│   ├── Doxyfile
│   └── README.md
│
├── .gitignore
├── CMakeLists.txt
├── README.md
└── LICENSE
```

## <a id="development"></a>🛠️ Development

The from-source install above builds the library only. To work on
MiniDB itself — running tests, benchmarks, or the regression tool —
build with everything enabled (the default):

```bash
cmake -B build
cmake --build build
```

**Run the test suite:**

```bash
ctest --test-dir build
```

**Run benchmarks and check for regressions:**

```bash
./build/benchmarks
./build/regression                  # latest baseline vs. benchmarks/results/benchmark_results.json
./build/regression v1.2.0           # a specific baseline vs. current
./build/regression v1.2.0 v1.4.0    # two baselines against each other
```

`regression` picks the latest baseline by semantic version (`v1.10.0`
correctly outranks `v1.9.0`), not alphabetical filename order, and
auto-names its output (`regression_v1.2.0_vs_current.md`/`.json`, etc.).

See [packaging/README.md](packaging/README.md) for notes on verifying the vcpkg
port and Conan recipe locally.

## <a id="benchmarks"></a>📊 Benchmarks

Unlike this author's other libraries, MiniDB has no natural drop-in
standard-library equivalent to benchmark against (there's no `std::`
embedded database) — these are absolute measurements, not a comparison.
Full results across every subsystem and scale: `benchmarks/results/v1_0_0.md`.

| Operation | MiniDB(1M) |
|---|---|
| StorageEngine FetchPage (hit) | 53.92 ms |
| StorageEngine FetchPage (miss) | 5.22 s |
| Table GetRecord (single page) | 72.69 ms |
| Table GetRecord (1000 pages) | 22.28 ms |
| Database GetTable (200 tables) | 21.16 ms |
| Database CreateTable | 311.39 ms |
| Page AddRecord (fill page) | 3.59 s |
| FileIO WriteSlot | 4.13 s |
| Record ToJson/fromJson | 664.34 ms |
| Record Move Ctor | 252.84 ms |

The page cache is doing real work: a `FetchPage` hit is roughly **~100x
faster** than a miss at 1M — that gap is the entire argument for the
buffer pool existing. `Table GetRecord` is also, somewhat
counterintuitively, *faster* per lookup when a table spans more pages
(22.28 ms across 1000 pages vs. 72.69 ms on a single page) — the O(1)
`pageIndex` lookup means page count stops being a cost variable in the
lookup path at all.

Separately, at the smaller iteration counts those operations are
actually measured at (real file writes, thread-pool runs, and sorts are
too expensive to benchmark at 1M reps): `QueryEngine`'s sorted select is
the single most expensive query operation, roughly 17x its unsorted
equivalent at the same 100-rep scale (275.98 ms vs. 16.69 ms) —
expected given `std::sort`'s O(n log n) versus a linear predicate scan,
but worth knowing before sorting a large result set on a
latency-sensitive path.

## <a id="documentation"></a>📖 Documentation

Full API reference, generated with Doxygen from `docs/Doxyfile`:

**https://privateMwb.github.io/MiniDatabase/**

## <a id="contributing"></a>🤝 Contributing

Issues and pull requests are welcome. Before submitting a PR:

- Run the test suite (`ctest --test-dir build`)
- If you're changing a hot path, run `./build/regression` and mention
  the results in your PR description

## <a id="changelog"></a>📝 Changelog

See the [Releases](https://github.com/privateMwb/MiniDatabase/releases)
page for version history and release notes.

## <a id="license"></a>📄 License

MIT — see [LICENSE](LICENSE) for details.
