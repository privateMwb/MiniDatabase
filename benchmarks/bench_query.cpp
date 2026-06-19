// Query Benchmark Suite
// Measures performance of QueryEngine operations against Table:
//
// - selectById (HashMap index lookup) as table size grows
// - selectAll full table scan as table size grows
// - filtered select with a single predicate as table size grows
// - aggregate functions (sum, avg, max, min) as table size grows
//
// This benchmark exercises the real QueryEngine path end to end,
// using ArenaAllocator for query scratch space exactly as it would
// be used in production.

#include <iostream>
#include <chrono>

#include "../include/Core/Table.h"
#include "../include/Engine/QueryEngine.h"
#include "../libs/CustomAllocators/ArenaAllocator/ArenaAllocator.h"
#include "../libs/VectorPro/VectorPro.h"

#include "utils/BenchTable.h"

// returns elapsed microseconds for a callable
template<typename F>
auto duration(F func) {
    auto start = std::chrono::steady_clock::now();
    func();
    auto end   = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

// prevents the compiler from eliminating unused operations
template<typename T>
inline void doNotOptimize(const T& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(value) : "memory");
#else
    volatile const T* p = &value;
    (void)p;
#endif
}

// Builds the standard employee schema used across this benchmark.
static VectorPro<ColumnDef> makeSchema() {
    VectorPro<ColumnDef> schema;
    schema.push_back({"name",   ColumnType::STRING, false});
    schema.push_back({"salary", ColumnType::DOUBLE, false});
    return schema;
}

// Builds a single test record with the given id.
static Record makeRecord(RecordID id) {
    Json data(Json::ObjectType{});
    data["name"]   = "Name" + std::to_string(id);
    data["salary"] = static_cast<double>(id) * 100.0;
    return Record(id, data);
}

// Builds a table pre-filled with the given number of records.
static Table makeFilledTable(std::size_t size) {
    Table table("bench", 0, makeSchema());
    for (std::size_t i = 0; i < size; ++i) {
        (void)table.insertRecord(makeRecord(i));
    }
    return table;
}

// Select By Id
// measures HashMap index lookup time as table size grows
void selectByIdBenchmark() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;

    for (std::size_t size : sizes) {
        Table          table = makeFilledTable(size);
        ArenaAllocator arena(DBConstants::ARENA_SIZE);
        QueryEngine    qe(arena);

        std::size_t lookups = 10'000;

        auto t = duration([&] {
            for (std::size_t i = 0; i < lookups; ++i) {
                RecordID    id     = i % size;
                QueryResult result = qe.selectByID(table, id);
                doNotOptimize(result.records.size());
            }
        });

        times.push_back(t.count());
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(benchtable::convert(sizes));
    data.push_back(benchtable::convert(times, "us"));

    VectorPro<std::string> headers;
    headers.push_back("Table Size");
    headers.push_back("10k Lookups");

    benchtable::table("Select By Id (HashMap Index)", headers, data, 56);
}

// Select All
// measures full table scan time as table size grows
void selectAllBenchmark() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;

    for (std::size_t size : sizes) {
        Table          table = makeFilledTable(size);
        ArenaAllocator arena(DBConstants::ARENA_SIZE);
        QueryEngine    qe(arena);

        auto t = duration([&] {
            QueryResult result = qe.selectAll(table);
            doNotOptimize(result.records.size());
        });

        times.push_back(t.count());
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(benchtable::convert(sizes));
    data.push_back(benchtable::convert(times, "us"));

    VectorPro<std::string> headers;
    headers.push_back("Table Size");
    headers.push_back("Scan Time");

    benchtable::table("Select All (Full Scan)", headers, data, 56);
}

// Filtered Select
// measures select() with a single GT predicate as table size grows
void filteredSelectBenchmark() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;

    for (std::size_t size : sizes) {
        Table          table = makeFilledTable(size);
        ArenaAllocator arena(DBConstants::ARENA_SIZE);
        QueryEngine    qe(arena);

        FilterPredicate gtFilter{"salary", Op::GT, Json(static_cast<double>(size) * 50.0)};

        VectorPro<FilterPredicate*> predicates;
        predicates.push_back(&gtFilter);

        auto t = duration([&] {
            QueryResult result = qe.select(table, predicates);
            doNotOptimize(result.records.size());
        });

        times.push_back(t.count());
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(benchtable::convert(sizes));
    data.push_back(benchtable::convert(times, "us"));

    VectorPro<std::string> headers;
    headers.push_back("Table Size");
    headers.push_back("Filter Time");

    benchtable::table("Filtered Select (salary > threshold)", headers, data, 56);
}

// Aggregates
// measures sum, avg, max, min cost at a fixed table size
void aggregatesBenchmark() {
    std::size_t size = 100'000;
    Table          table = makeFilledTable(size);
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    VectorPro<FilterPredicate*> predicates;

    VectorPro<std::string> labels;
    VectorPro<long>        times;

    {
        auto t = duration([&] {
            double result = qe.sum(table, "salary", predicates);
            doNotOptimize(result);
        });
        labels.push_back("sum");
        times.push_back(t.count());
    }

    {
        auto t = duration([&] {
            double result = qe.avg(table, "salary", predicates);
            doNotOptimize(result);
        });
        labels.push_back("avg");
        times.push_back(t.count());
    }

    {
        auto t = duration([&] {
            double result = qe.max(table, "salary", predicates);
            doNotOptimize(result);
        });
        labels.push_back("max");
        times.push_back(t.count());
    }

    {
        auto t = duration([&] {
            double result = qe.min(table, "salary", predicates);
            doNotOptimize(result);
        });
        labels.push_back("min");
        times.push_back(t.count());
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(labels);
    data.push_back(benchtable::convert(times, "us"));

    VectorPro<std::string> headers;
    headers.push_back("Aggregate");
    headers.push_back("Time");

    benchtable::table("Aggregates (100,000 records)", headers, data, 56);
}

// Entry Point
int run_query_benchmarks() {
    selectByIdBenchmark();
    selectAllBenchmark();
    filteredSelectBenchmark();
    aggregatesBenchmark();

    return 0;
}