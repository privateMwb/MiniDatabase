// Insert Benchmark Suite
// Measures performance of record insertion into Table:
//
// - insert throughput as record count grows
// - average time per insert at each scale
//
// This benchmark exercises Table::insertRecord end to end, which
// internally uses PoolAllocator for record storage and HashMap for
// index maintenance — measuring the real cost of a full insert
// through the engine, not an isolated allocator microbenchmark.

#include <iostream>
#include <chrono>

#include "../include/Core/Table.h"
#include "utils/BenchTable.h"
#include "../libs/VectorPro/VectorPro.h"

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

// Insert Throughput
// measures total time and records/sec as the dataset size grows
void insertThroughput() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;
    VectorPro<long> rates;

    for (std::size_t size : sizes) {
        auto t = duration([&] {
            Table table("bench", 0, makeSchema());
            for (std::size_t i = 0; i < size; ++i) {
                (void)table.insertRecord(makeRecord(i));
            }
            doNotOptimize(table.recordCount());
        });

        long micros = t.count();
        long recordsPerSec = micros > 0
            ? static_cast<long>((static_cast<double>(size) / micros) * 1'000'000.0)
            : 0;

        times.push_back(micros);
        rates.push_back(recordsPerSec);
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(benchtable::convert(sizes));
    data.push_back(benchtable::convert(times, "us"));
    data.push_back(benchtable::convert(rates, "rec/s"));

    VectorPro<std::string> headers;
    headers.push_back("Records");
    headers.push_back("Time");
    headers.push_back("Rate");

    benchtable::table("Insert Throughput", headers, data, 56);
}

// Average Insert Cost
// measures the average per-record cost of insertRecord at each scale
void averageInsertCost() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> avgNanosPerInsert;

    for (std::size_t size : sizes) {
        auto t = duration([&] {
            Table table("bench", 0, makeSchema());
            for (std::size_t i = 0; i < size; ++i) {
                (void)table.insertRecord(makeRecord(i));
            }
            doNotOptimize(table.recordCount());
        });

        long micros = t.count();
        long avgNs  = static_cast<long>((static_cast<double>(micros) * 1000.0) / static_cast<double>(size));

        avgNanosPerInsert.push_back(avgNs);
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(benchtable::convert(sizes));
    data.push_back(benchtable::convert(avgNanosPerInsert, "ns/insert"));

    VectorPro<std::string> headers;
    headers.push_back("Records");
    headers.push_back("Avg Cost");

    benchtable::table("Average Insert Cost", headers, data, 56);
}

// Entry Point
void run_insert_benchmarks() {
    insertThroughput();
    averageInsertCost();
}