// ============================================================
// fuzz/fuzz_wal_recovery.cpp
//
// Fuzzer for WriteAheadLog::open()'s crash-recovery scan, targeting
// the on-disk log file directly -- the whole point of recovery is to
// survive exactly the kind of arbitrary corruption a crash mid-write
// leaves behind, so raw fuzzer bytes are a realistic proxy for "the
// file as some previous, interrupted process left it," not an
// artificial input shape. Unlike fuzz_record_deserialize.cpp's JSON
// text, this is MiniDatabase's other untrusted-bytes-from-disk
// surface: a binary, CRC32-framed format read directly, with no
// Json::parse() involved anywhere in the path.
//
// This codebase's own regression/unit coverage already has two fixed
// corruption shapes -- a truncated trailing record and a checksum-
// mismatched trailing record (see
// tests/*/suite/{regression,unit}/write_ahead_log.cpp) -- both
// hand-crafted at exact byte offsets. This harness is the open-ended
// counterpart to those two fixed cases: arbitrary corruption, at any
// offset, of any shape.
//
// Not a differential fuzzer (no shadow-model WAL implementation exists
// to compare against); instead checks the one invariant open()'s
// recovery contract implies: whatever lastIndex() open() settles on
// after scanning a possibly-corrupt file, every entry from 1 through
// that lastIndex() must actually be readable back out cleanly. A
// recovery scan that reports a lastIndex() it can't actually back up
// with real, intact entries is the specific failure mode this harness
// exists to catch.
//
// ASSUMPTION: WriteAheadLog's constructor takes a file path and does
// not itself require the file to exist beforehand (open() creates it
// if missing, per WriteAheadLog.h) -- matching the recovery-of-a-
// possibly-partial-file design this harness is fuzzing. Not verified
// against a live build.
// ============================================================

#include <MiniDB/MiniDatabase.h>
#include <MiniDB/Storage/WriteAheadLog.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace MiniDB::Storage;
using namespace MiniDB::Common;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // A unique path per process (not per call: libFuzzer runs each
    // input through the same process in normal fuzzing mode, so this
    // only needs to avoid colliding with a /different/ fuzzer process,
    // e.g. under parallel corpus minimization).
    const std::string path = "/tmp/fuzz_wal_recovery_" + std::to_string(getpid()) + ".wal";

    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    {
        WriteAheadLog log(path);
        Status openStatus = log.open();

        if (openStatus == Status::OK) {
            LogIndex last = log.lastIndex();
            if (last != INVALID_LOG_INDEX) {
                // The recovery contract: everything open() claims
                // survived must actually read back out, from 1 through
                // lastIndex(), with no gaps and no out-of-bounds reads.
                Vector<LogEntry> entries;
                Status rangeStatus = log.range(1, last, entries);
                if (rangeStatus != Status::OK || entries.size() != last) {
                    __builtin_trap();
                }

                LogEntry single;
                if (log.entryAt(last, single) != Status::OK) {
                    __builtin_trap();
                }
            }
        }
        // openStatus != Status::OK is an acceptable, documented outcome
        // for a file recovery genuinely can't make sense of -- not
        // every possible byte sequence is required to recover into
        // something usable, only to fail safely rather than crash or
        // silently fabricate entries that were never durably written.
    }

    std::remove(path.c_str());
    return 0;
}
