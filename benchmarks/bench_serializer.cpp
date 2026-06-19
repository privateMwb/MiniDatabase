// Serializer Benchmark Suite
// Measures performance of JSON export and import:
//
// - export throughput as table size grows
// - import throughput as table size grows
// - round-trip cost (export + import combined)
//
// This benchmark exercises the real Serializer path end to end,
// including JsonParser's dump() and parse() under realistic
// table sizes.

#include <iostream>
#include <chrono>

#include "../include/Core/Table.h"
#include "../include/Engine/Serializer.h"
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
    Json fields(Json::ObjectType{});
    fields["name"]   = "Name" + std::to_string(id);
    fields["salary"] = static_cast<double>(id) * 100.0;
    return Record(id, fields);
}

// Builds a table pre-filled with the given number of records.
static Table makeFilledTable(std::size_t size) {
    Table table("bench", 0, makeSchema());
    for (std::size_t i = 0; i < size; ++i) {
        (void)table.insertRecord(makeRecord(i));
    }
    return table;
}

// Export Throughput
// measures exportTableToJson time and records/sec as table size grows
void exportThroughputBenchmark() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;
    VectorPro<long> rates;

    for (std::size_t size : sizes) {
        Table table = makeFilledTable(size);

        auto t = duration([&] {
            std::string json = Serializer::exportTableToJson(table);
            doNotOptimize(json.size());
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

    benchtable::table("Export Throughput", headers, data, 56);
}

// Import Throughput
// measures importTableFromJson time and records/sec as table size grows
void importThroughputBenchmark() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;
    VectorPro<long> rates;

    for (std::size_t size : sizes) {
        Table       source = makeFilledTable(size);
        std::string json   = Serializer::exportTableToJson(source);

        auto t = duration([&] {
            Table target("bench", 0, makeSchema());
            Status s = Serializer::importTableFromJson(target, json);
            doNotOptimize(s);
            doNotOptimize(target.recordCount());
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

    benchtable::table("Import Throughput", headers, data, 56);
}

// Round Trip
// measures combined export + import cost as table size grows
void roundTripBenchmark() {
    VectorPro<std::size_t> sizes;
    sizes.push_back(1'000);
    sizes.push_back(10'000);
    sizes.push_back(100'000);

    VectorPro<long> times;

    for (std::size_t size : sizes) {
        Table source = makeFilledTable(size);

        auto t = duration([&] {
            std::string json = Serializer::exportTableToJson(source);

            Table target("bench", 0, makeSchema());
            Status s = Serializer::importTableFromJson(target, json);

            doNotOptimize(s);
            doNotOptimize(target.recordCount());
        });

        times.push_back(t.count());
    }

    VectorPro<VectorPro<std::string>> data;
    data.push_back(benchtable::convert(sizes));
    data.push_back(benchtable::convert(times, "us"));

    VectorPro<std::string> headers;
    headers.push_back("Records");
    headers.push_back("Round Trip Time");

    benchtable::table("Round Trip (Export + Import)", headers, data, 56);
}

// Entry Point
int run_serializer_benchmarks() {
    exportThroughputBenchmark();
    importThroughputBenchmark();
    roundTripBenchmark();

    return 0;
}

