// WriteAheadLog Benchmark Suite
// Measures the append-only durable log primitive: durable append (fsync
// per call), indexed reads, the open()-time recovery scan, and
// truncation.
//
// Covers:
// - append() (fsync every call)
// - entryAt() / range() reads against a populated log
// - open() re-scanning an already-populated, uncorrupted log
// - truncateFrom() (measured together with the append that replenishes
//   the entry it removes -- see truncate_from for why)

#include <MiniDB/MiniDatabase.h>
#include <MiniDB/Storage/WriteAheadLog.h>
#include <benchmark/benchmark.h>

#include <filesystem>

using namespace MiniDB::Storage;
using namespace MiniDB::Common;

namespace {

// Appends `count` entries to `log`.
void populate(WriteAheadLog& log, int count) {
    LogIndex idx = INVALID_LOG_INDEX;
    for (int i = 0; i < count; ++i) {
        (void)log.append(1, "benchmark-payload", idx);
    }
}

} // namespace

// Measures append() durably writing a fresh entry -- fsync every call,
// so this sits closer to FileIO::writeFileAtomic's cost profile than to
// writeSlot's (no fsync). Cost should stay flat regardless of current
// log size: append() is O(1), writing at writeOffset_ with no scan.
static void wal_append(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "wal_append";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/wal.log";

    WriteAheadLog log(path);
    (void)log.open();

    LogIndex idx = INVALID_LOG_INDEX;
    for (auto _ : state) {
        (void)log.append(1, "benchmark-payload", idx);
    }

    fs::remove_all(dir);
}
BENCHMARK(wal_append);

// Measures entryAt() reading a fixed index from a log with 1000 existing
// entries.
static void wal_entryAt(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "wal_entry_at";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/wal.log";

    WriteAheadLog log(path);
    (void)log.open();
    populate(log, 1000);

    LogEntry out;
    for (auto _ : state) {
        (void)log.entryAt(500, out);
        benchmark::DoNotOptimize(out);
    }

    fs::remove_all(dir);
}
BENCHMARK(wal_entryAt);

// Measures range() reading a 100-entry span from a log with 1000
// existing entries.
static void wal_range(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "wal_range";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/wal.log";

    WriteAheadLog log(path);
    (void)log.open();
    populate(log, 1000);

    Vector<LogEntry> out;
    for (auto _ : state) {
        (void)log.range(401, 500, out);
        benchmark::DoNotOptimize(out);
    }

    fs::remove_all(dir);
}
BENCHMARK(wal_range);

// Measures open() re-scanning an already-populated, uncorrupted
// 1000-entry log -- the recovery-scan cost paid on every startup, not
// just after a crash.
static void wal_openRecoveryScan(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "wal_open_scan";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/wal.log";

    {
        WriteAheadLog seed(path);
        (void)seed.open();
        populate(seed, 1000);
    } // fully durable on disk before the benchmark loop starts

    WriteAheadLog log(path);
    for (auto _ : state) {
        (void)log.open();
    }

    fs::remove_all(dir);
}
BENCHMARK(wal_openRecoveryScan);

// Measures truncateFrom() removing the single most recent entry. Each
// iteration first appends a throwaway entry, then truncates exactly that
// entry back off, so the log's size stays constant and the benchmark
// stays valid for any number of iterations -- unlike calling
// truncateFrom() alone against a fixed pre-populated log, which would
// only be a real truncation on the first call and a same-cost no-op on
// every call after (index > lastIndex() once the log is exhausted). This
// measures append()+truncateFrom() together, not truncateFrom() in
// isolation; isolating it would need a per-iteration setup/teardown hook
// that benchmark::State::PauseTiming()/ResumeTiming() could provide, but
// that's left as a follow-up rather than assumed here.
static void wal_truncateFrom(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string dir = "wal_truncate";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string path = dir + "/wal.log";

    WriteAheadLog log(path);
    (void)log.open();

    for (auto _ : state) {
        LogIndex idx = INVALID_LOG_INDEX;
        (void)log.append(1, "x", idx);
        (void)log.truncateFrom(idx);
    }

    fs::remove_all(dir);
}
BENCHMARK(wal_truncateFrom);