// FileIO Benchmark Suite
// Measures the raw file I/O primitives: fixed-size slot read/write
// (StorageEngine's page persistence layer) and atomic whole-file
// write/read (Database::save/load's persistence layer).
//
// Covers:
// - writeSlot / readSlot round trip
// - writeFileAtomic / readFile round trip

#include <support/framework.h>

#include <filesystem>

using namespace MiniDB::Common;

// Measures writeSlot() overwriting the same fixed slot repeatedly.
static void bench_write_slot() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_fileio_write_slot";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/slots.bin";

    std::string payload(200, 'x');

    auto write = [&] { (void)FileIO::writeSlot(path, 0, DBConstants::PAGE_SIZE, payload); };
    BENCH("FileIO writeSlot", write);

    fs::remove_all(dir);
}

// Measures readSlot() reading a slot that already exists.
static void bench_read_slot() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_fileio_read_slot";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/slots.bin";

    std::string payload(200, 'x');
    (void)FileIO::writeSlot(path, 0, DBConstants::PAGE_SIZE, payload);

    std::string out;
    auto read = [&] {
        (void)FileIO::readSlot(path, 0, DBConstants::PAGE_SIZE, out);
        doNotOptimize(out);
    };
    BENCH("FileIO readSlot", read);

    fs::remove_all(dir);
}

// Measures writeFileAtomic() overwriting the same file repeatedly (temp
// file create + write + fsync + rename, every call).
static void bench_write_atomic() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_fileio_write_atomic";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/db.json";

    std::string content(2000, 'x');

    auto write = [&] { (void)FileIO::writeFileAtomic(path, content); };
    BENCH_CUSTOM("FileIO writeFileAtomic", write);

    fs::remove_all(dir);
}

// Measures readFile() reading back a previously written file.
static void bench_read_file() {
    namespace fs = std::filesystem;
    const std::string dir = "bench_fileio_read_file";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/db.json";

    std::string content(2000, 'x');
    (void)FileIO::writeFileAtomic(path, content);

    std::string out;
    auto read = [&] {
        (void)FileIO::readFile(path, out);
        doNotOptimize(out);
    };
    BENCH("FileIO readFile", read);

    fs::remove_all(dir);
}

// Executes all FileIO benchmark cases.
static void run_benchmarks() {
    bench_write_slot();
    std::cout << "\n";

    bench_read_slot();
    std::cout << "\n";

    bench_write_atomic();
    std::cout << "\n";

    bench_read_file();
}

REGISTER_BENCH_SUITE();