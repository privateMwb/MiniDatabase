// WriteAheadLog Test Suite
// Verifies durable append, indexed reads, truncation, and crash recovery
// for MiniDB::Storage::WriteAheadLog.
//
// Covers:
// - open() on a missing file (creates it) and on a clean-shutdown reopen
// - append() assigning sequential indices
// - entryAt / range byte-exact reads, including range's inclusive bounds
// - truncateFrom reusing the truncated index rather than continuing past
//   the old end (regression coverage), truncateFrom(0), and the
//   already-satisfied no-op case
// - crash recovery discarding a truncated trailing record and a
//   checksum-mismatched trailing record, each hand-corrupted on disk
// - empty-file and single-entry edge cases

#include <support/framework.h>

#include <MiniDB/Storage/WriteAheadLog.h>

#include <filesystem>
#include <fstream>

using namespace MiniDB::Storage;
using namespace MiniDB::Common;

namespace {

// Returns a fresh temp file path for this test run; not created yet.
std::string tempPath(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path();
    return (dir / ("minidb_test_" + label + ".wal")).string();
}

// Flips a single byte at `offset` in the file at `path`, simulating
// on-disk corruption that happens to leave frameLength/payloadLength
// internally consistent (as opposed to a truncated write -- see
// corruptByTruncating() below).
void corruptByFlippingByte(const std::string& path, std::uint64_t offset) {
    std::fstream f(path, std::ios::in | std::ios::out | std::ios::binary);
    CHK(f.is_open());
    f.seekg(static_cast<std::streamoff>(offset));
    char byte = 0;
    f.read(&byte, 1);
    f.seekp(static_cast<std::streamoff>(offset));
    byte = static_cast<char>(byte ^ 0xFF);
    f.write(&byte, 1);
}

// Chops the file at `path` down to `size` bytes, simulating a crash
// mid-write (a frame whose frameLength claims more bytes than the file
// has).
void corruptByTruncating(const std::string& path, std::uint64_t size) {
    std::filesystem::resize_file(path, size);
}

} // namespace

// Verifies open() creates a missing file and the log starts empty.
static void open_creates_empty() {
    std::string path = tempPath("open_empty");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    CHK(log.lastIndex() == INVALID_LOG_INDEX);
    CHK(log.lastTerm() == INVALID_TERM);
    CHK(std::filesystem::exists(path) == true);

    std::filesystem::remove(path);
}

// Verifies append() assigns sequential indices and tracks the last term.
static void append_sequential_indices() {
    std::string path = tempPath("append_sequential");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);

    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(1, "first", idx) == Status::OK);
    CHK(idx == 1u);
    CHK(log.append(1, "second", idx) == Status::OK);
    CHK(idx == 2u);

    CHK(log.lastIndex() == 2u);
    CHK(log.lastTerm() == 1u);

    std::filesystem::remove(path);
}

// Verifies a clean-shutdown reopen preserves lastIndex/lastTerm.
static void reopen_preserves_state() {
    std::string path = tempPath("reopen_clean");
    std::filesystem::remove(path);

    {
        WriteAheadLog log(path);
        CHK(log.open() == Status::OK);
        LogIndex idx = INVALID_LOG_INDEX;
        CHK(log.append(3, "a", idx) == Status::OK);
        CHK(log.append(5, "b", idx) == Status::OK);
    } // destructor closes the fd

    WriteAheadLog reopened(path);
    CHK(reopened.open() == Status::OK);
    CHK(reopened.lastIndex() == 2u);
    CHK(reopened.lastTerm() == 5u);

    std::filesystem::remove(path);
}

// Verifies entryAt() returns the payload byte-exact -- including an
// embedded '\0' and a non-ASCII byte, proving payload is handled by
// length, not treated as a null-terminated C string anywhere.
static void entry_at_byte_exact() {
    std::string path = tempPath("entry_at_byte_exact");
    std::filesystem::remove(path);
    const std::string payload("head\0mid\xFF tail", 14);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(7, payload, idx) == Status::OK);

    LogEntry out;
    CHK(log.entryAt(idx, out) == Status::OK);
    CHK(out.index == idx);
    CHK(out.term == 7u);
    CHK(out.payload == payload);

    std::filesystem::remove(path);
}

// Verifies range() is inclusive on both ends.
static void range_inclusive_ends() {
    std::string path = tempPath("range_inclusive");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    LogIndex idx = INVALID_LOG_INDEX;
    for (int i = 0; i < 5; ++i) {
        CHK(log.append(1, "entry-" + std::to_string(i), idx) == Status::OK);
    }

    Vector<LogEntry> out;
    CHK(log.range(2, 4, out) == Status::OK);
    CHK(out.size() == 3u);
    CHK(out[0].payload == "entry-1"); // index 2
    CHK(out[1].payload == "entry-2"); // index 3
    CHK(out[2].payload == "entry-3"); // index 4

    std::filesystem::remove(path);
}

// Verifies range() past lastIndex() fails and leaves out cleared, not
// partially filled.
static void range_past_last_not_found() {
    std::string path = tempPath("range_past_last");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(1, "only", idx) == Status::OK);

    Vector<LogEntry> out;
    out.push_back(LogEntry{}); // pre-populate: must be cleared even on failure
    CHK(log.range(1, 5, out) == Status::NOT_FOUND);
    CHK(out.size() == 0u);

    std::filesystem::remove(path);
}

// Regression: truncateFrom() followed by append() must reuse the
// truncated index, not continue counting from the log's old end -- and
// the discarded entries must be genuinely gone, not merely shadowed.
static void truncate_append_lands_index() {
    std::string path = tempPath("truncate_then_append");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);

    LogIndex idx = INVALID_LOG_INDEX;
    for (int i = 0; i < 5; ++i) {
        CHK(log.append(1, "old-" + std::to_string(i), idx) == Status::OK);
    }
    CHK(log.lastIndex() == 5u);

    // Discard entries 3..5, keeping 1..2.
    CHK(log.truncateFrom(3) == Status::OK);
    CHK(log.lastIndex() == 2u);
    CHK(log.lastTerm() == 1u);

    CHK(log.append(9, "new-3", idx) == Status::OK);
    CHK(idx == 3u); // reuses index 3, does not continue from the old 5

    LogEntry out;
    CHK(log.entryAt(3, out) == Status::OK);
    CHK(out.payload == "new-3"); // the old entry 3 is gone, not just shadowed
    CHK(out.term == 9u);

    CHK(log.entryAt(4, out) == Status::NOT_FOUND); // old entries 4-5 unreachable

    std::filesystem::remove(path);
}

// Verifies truncateFrom(0) wipes every entry and the log remains
// usable afterward. The file itself does NOT go to 0 bytes -- it's
// truncated back to exactly its freshly-opened, zero-entry size (the
// magic header alone), not an unrecognized empty file. See
// WriteAheadLog.cpp's MAGIC doc comment: truncating all the way to 0
// would regress the file out of the "stamped, recognized WAL" state
// this class's open() now requires of any non-empty file.
static void truncate_zero_wipes_log() {
    std::string path = tempPath("truncate_zero");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    const auto emptyLogSize = std::filesystem::file_size(path); // header-only baseline

    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(1, "a", idx) == Status::OK);
    CHK(log.append(1, "b", idx) == Status::OK);

    CHK(log.truncateFrom(INVALID_LOG_INDEX) == Status::OK);
    CHK(log.lastIndex() == INVALID_LOG_INDEX);
    CHK(log.lastTerm() == INVALID_TERM);
    CHK(std::filesystem::file_size(path) == emptyLogSize);

    CHK(log.append(2, "fresh", idx) == Status::OK);
    CHK(idx == 1u);

    std::filesystem::remove(path);
}

// Verifies truncateFrom() past lastIndex() is a no-op, not an error.
static void truncate_beyond_noop() {
    std::string path = tempPath("truncate_beyond");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(1, "a", idx) == Status::OK);
    CHK(log.append(1, "b", idx) == Status::OK);
    CHK(log.append(1, "c", idx) == Status::OK);

    CHK(log.truncateFrom(10) == Status::OK); // nothing at/after 10 to begin with
    CHK(log.lastIndex() == 3u);

    LogEntry out;
    CHK(log.entryAt(3, out) == Status::OK);
    CHK(out.payload == "c"); // untouched

    std::filesystem::remove(path);
}

// Verifies open() discards a truncated trailing record (a crash partway
// through an append) and leaves the log usable afterward.
static void recovery_discards_truncated() {
    std::string path = tempPath("recovery_truncated");
    std::filesystem::remove(path);
    std::uint64_t sizeAfterTwo = 0;

    {
        WriteAheadLog log(path);
        CHK(log.open() == Status::OK);
        LogIndex idx = INVALID_LOG_INDEX;
        CHK(log.append(1, "one", idx) == Status::OK);
        CHK(log.append(1, "two", idx) == Status::OK);
        sizeAfterTwo = std::filesystem::file_size(path);

        CHK(log.append(1, "three-not-fully-durable", idx) == Status::OK);
    } // full, valid file on disk at this point

    const std::uint64_t fullSize = std::filesystem::file_size(path);
    CHK(fullSize > sizeAfterTwo);

    // Simulate a crash partway through writing the third record: chop the
    // file to a size that includes only part of that record's frame.
    corruptByTruncating(path, sizeAfterTwo + 10);

    WriteAheadLog recovered(path);
    CHK(recovered.open() == Status::OK);
    CHK(recovered.lastIndex() == 2u); // only the two fully-durable entries survive
    CHK(std::filesystem::file_size(path) == sizeAfterTwo); // partial tail dropped from disk too

    LogEntry out;
    CHK(recovered.entryAt(2, out) == Status::OK);
    CHK(out.payload == "two");

    // Log remains fully usable after recovery: the next append reuses
    // index 3 rather than leaving a gap.
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(recovered.append(1, "three-again", idx) == Status::OK);
    CHK(idx == 3u);

    std::filesystem::remove(path);
}

// Verifies open() discards a checksum-mismatched trailing record (an
// intact-length but corrupted record) rather than trusting its length
// prefix alone.
static void recovery_discards_checksum() {
    std::string path = tempPath("recovery_checksum");
    std::filesystem::remove(path);
    std::uint64_t sizeAfterTwo = 0;

    {
        WriteAheadLog log(path);
        CHK(log.open() == Status::OK);
        LogIndex idx = INVALID_LOG_INDEX;
        CHK(log.append(1, "one", idx) == Status::OK);
        CHK(log.append(1, "two", idx) == Status::OK);
        sizeAfterTwo = std::filesystem::file_size(path);

        CHK(log.append(1, "three-payload", idx) == Status::OK);
    }

    // The third record's frame is [frameLength(4)][index(8)][term(8)]
    // [payloadLength(4)][payload]...; flip a byte inside its payload,
    // well past the header, so the frame still looks structurally
    // consistent (frameLength matches payloadLength) but the checksum
    // -- which covers the payload too -- no longer matches.
    corruptByFlippingByte(path, sizeAfterTwo + 4 + 8 + 8 + 4 + 2);

    WriteAheadLog recovered(path);
    CHK(recovered.open() == Status::OK);
    CHK(recovered.lastIndex() ==
        2u); // the corrupt third record is discarded, not just the flipped byte
    CHK(std::filesystem::file_size(path) == sizeAfterTwo);

    std::filesystem::remove(path);
}

// Verifies open() on a zero-byte file succeeds cleanly.
static void recovery_empty_file() {
    std::string path = tempPath("recovery_empty");
    std::filesystem::remove(path);
    { std::ofstream f(path, std::ios::binary); } // zero-byte file, deliberately

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    CHK(log.lastIndex() == INVALID_LOG_INDEX);

    std::filesystem::remove(path);
}

// Verifies entryAt() rejects INVALID_LOG_INDEX and any index past
// lastIndex() with NOT_FOUND.
static void entry_at_invalid_range() {
    std::string path = tempPath("entry_at_invalid");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(1, "only", idx) == Status::OK);

    LogEntry out;
    CHK(log.entryAt(INVALID_LOG_INDEX, out) == Status::NOT_FOUND);
    CHK(log.entryAt(2, out) == Status::NOT_FOUND); // one past lastIndex()

    std::filesystem::remove(path);
}

// Verifies a single-entry log round trips correctly and reports the
// right lastTerm().
static void single_entry_round_trip() {
    std::string path = tempPath("single_entry");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(42, "solo", idx) == Status::OK);

    CHK(log.lastIndex() == 1u);
    CHK(log.lastTerm() == 42u);

    Vector<LogEntry> out;
    CHK(log.range(1, 1, out) == Status::OK);
    CHK(out.size() == 1u);
    CHK(out[0].payload == "solo");

    std::filesystem::remove(path);
}

// Verifies open() refuses a non-empty file that was never created by
// this class (no magic header), leaving it byte-for-byte untouched --
// rather than treating "unrecognized file" the same as "corrupted
// WAL" and truncating it, as an earlier version of this class did.
static void open_refuses_foreign_file() {
    std::string path = tempPath("foreign_file");
    std::filesystem::remove(path);

    const std::string originalContents = "This file belongs to something else entirely.";
    {
        std::ofstream out(path, std::ios::binary);
        out << originalContents;
    }

    WriteAheadLog log(path);
    CHK(log.open() == Status::PARSE_ERROR);

    std::ifstream in(path, std::ios::binary);
    std::string afterOpen((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHK(afterOpen == originalContents); // completely untouched, not truncated

    std::filesystem::remove(path);
}

// Verifies open() also refuses a file shorter than the magic header
// itself, without touching it.
static void open_refuses_too_short_file() {
    std::string path = tempPath("too_short");
    std::filesystem::remove(path);

    {
        std::ofstream out(path, std::ios::binary);
        out << "hi"; // shorter than the 8-byte magic header
    }

    WriteAheadLog log(path);
    CHK(log.open() == Status::PARSE_ERROR);
    CHK(std::filesystem::file_size(path) == 2u);

    std::filesystem::remove(path);
}

// Verifies append() rejects a payload over MAX_PAYLOAD_SIZE instead of
// silently wrapping the on-disk uint32_t frame-length field.
static void append_rejects_oversized_payload() {
    std::string path = tempPath("oversized_payload");
    std::filesystem::remove(path);

    WriteAheadLog log(path);
    CHK(log.open() == Status::OK);

    std::string oversized(MAX_PAYLOAD_SIZE + 1, 'x');
    LogIndex idx = INVALID_LOG_INDEX;
    CHK(log.append(1, oversized, idx) == Status::INVALID_TYPE);
    CHK(log.lastIndex() == INVALID_LOG_INDEX); // rejected before anything was written

    CHK(log.append(1, "normal entry", idx) == Status::OK);
    CHK(idx == 1u);

    std::filesystem::remove(path);
}

// Verifies a full truncateFrom(0) leaves the file at its recognized,
// freshly-opened (header-only) size, not 0 bytes -- so a later,
// genuinely fresh open() still recognizes it as a WAL.
static void truncate_zero_keeps_file_reopenable() {
    std::string path = tempPath("truncate_zero_reopen");
    std::filesystem::remove(path);

    {
        WriteAheadLog log(path);
        CHK(log.open() == Status::OK);
        LogIndex idx = INVALID_LOG_INDEX;
        CHK(log.append(1, "a", idx) == Status::OK);
        CHK(log.truncateFrom(INVALID_LOG_INDEX) == Status::OK);
    }

    WriteAheadLog reopened(path);
    CHK(reopened.open() == Status::OK); // not PARSE_ERROR -- still a recognized WAL
    CHK(reopened.lastIndex() == INVALID_LOG_INDEX);

    std::filesystem::remove(path);
}

// Executes all WriteAheadLog test cases.
static void run_tests() {
    RUN(open_creates_empty);
    RUN(append_sequential_indices);
    RUN(reopen_preserves_state);
    RUN(entry_at_byte_exact);
    RUN(range_inclusive_ends);
    RUN(range_past_last_not_found);
    RUN(truncate_append_lands_index);
    RUN(truncate_zero_wipes_log);
    RUN(truncate_beyond_noop);
    RUN(recovery_discards_truncated);
    RUN(recovery_discards_checksum);
    RUN(recovery_empty_file);
    RUN(entry_at_invalid_range);
    RUN(single_entry_round_trip);
    RUN(open_refuses_foreign_file);
    RUN(open_refuses_too_short_file);
    RUN(append_rejects_oversized_payload);
    RUN(truncate_zero_keeps_file_reopenable);
}

REGISTER_TEST_SUITE();
