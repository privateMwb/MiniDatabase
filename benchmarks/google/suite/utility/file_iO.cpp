// FileIO Benchmark Suite
// Measures the raw file I/O primitives: fixed-size slot read/write
// (StorageEngine's page persistence layer) and atomic whole-file
// write/read (Database::save/load's persistence layer).
//
// Covers:
// - writeSlot / readSlot round trip
// - writeFileAtomic / readFile round trip

#include <MiniDB/MiniDatabase.h>
#include <benchmark/benchmark.h>

#include <filesystem>

using namespace MiniDB::Common;

// Measures writeSlot() overwriting the same fixed slot repeatedly.
static void write_slot(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "fileio_write_slot";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/slots.bin";

    std::string payload(200, 'x');

    for (auto _ : state) {
        (void)FileIO::writeSlot(path, 0, DBConstants::PAGE_SIZE, payload);
    }

    fs::remove_all(dir);
}
BENCHMARK(write_slot);

// Measures writeFileAtomic() overwriting the same file repeatedly (temp
// file create + write + fsync + rename, every call).
static void write_atomic(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "fileio_write_atomic";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/db.json";

    std::string content(2000, 'x');

    for (auto _ : state) {
        (void)FileIO::writeFileAtomic(path, content);
    }

    fs::remove_all(dir);
}
BENCHMARK(write_atomic);

// Measures readSlot() reading a slot that already exists.
static void read_slot(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "fileio_read_slot";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/slots.bin";

    std::string payload(200, 'x');
    (void)FileIO::writeSlot(path, 0, DBConstants::PAGE_SIZE, payload);

    std::string out;
    for (auto _ : state) {
        (void)FileIO::readSlot(path, 0, DBConstants::PAGE_SIZE, out);
        benchmark::DoNotOptimize(out);
    }

    fs::remove_all(dir);
}
BENCHMARK(read_slot);

// Measures readFile() reading back a previously written file.
static void read_file(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "fileio_read_file";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/db.json";

    std::string content(2000, 'x');
    (void)FileIO::writeFileAtomic(path, content);

    std::string out;
    for (auto _ : state) {
        (void)FileIO::readFile(path, out);
        benchmark::DoNotOptimize(out);
    }

    fs::remove_all(dir);
}
BENCHMARK(read_file);