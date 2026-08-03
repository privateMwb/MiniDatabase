// FileIO Test Suite
// Verifies MiniDB::Common::FileIO's file-level and fixed-slot I/O
// primitives directly, independent of Record/Page/Table/Database.
//
// Covers:
// - writeFileAtomic / readFile round trip
// - readFile on a missing file
// - writeSlot / readSlot round trip (single and multiple slots in the
//   same file, at non-adjacent indices)
// - writeSlot rejecting a payload that doesn't fit in the slot size
//   (OUT_OF_MEMORY, not silent truncation)
// - readSlot on a never-written slot / missing file (NOT_FOUND)
// - readSlot on a corrupt length prefix (PARSE_ERROR)

#include <support/framework.h>

#include <cstring>
#include <filesystem>
#include <fstream>

using namespace MiniDB::Common;
using namespace MiniDB::Common::FileIO;

namespace {

std::string tempPath(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path();
    return (dir / ("minidb_fileio_test_" + label)).string();
}

} // namespace

// Verifies writeFileAtomic followed by readFile returns the same content.
static void write_atomic_then_read_round_trip() {
    std::string path = tempPath("atomic_roundtrip");
    std::filesystem::remove(path);

    CHK(writeFileAtomic(path, "hello world") == Status::OK);

    std::string out;
    CHK(readFile(path, out) == Status::OK);
    CHK(out == "hello world");

    std::filesystem::remove(path);
}

// Verifies writeFileAtomic overwrites existing content rather than
// appending or leaving stale bytes behind.
static void write_atomic_overwrites_existing_content() {
    std::string path = tempPath("atomic_overwrite");
    std::filesystem::remove(path);

    CHK(writeFileAtomic(path, "aaaaaaaaaa") == Status::OK);
    CHK(writeFileAtomic(path, "bb") == Status::OK);

    std::string out;
    CHK(readFile(path, out) == Status::OK);
    CHK(out == "bb");

    std::filesystem::remove(path);
}

// Verifies writeFileAtomic does not leave a stray ".tmp" file behind after
// a successful write.
static void write_atomic_leaves_no_tmp_file() {
    std::string path = tempPath("atomic_no_tmp");
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".tmp");

    CHK(writeFileAtomic(path, "content") == Status::OK);
    CHK(std::filesystem::exists(path + ".tmp") == false);

    std::filesystem::remove(path);
}

// Verifies readFile on a nonexistent file returns IO_ERROR rather than
// crashing or returning an empty success.
static void read_missing_file_returns_io_error() {
    std::string path = tempPath("does_not_exist");
    std::filesystem::remove(path);

    std::string out;
    CHK(readFile(path, out) == Status::IO_ERROR);
}

// Verifies writeSlot followed by readSlot at the same index returns the
// same payload.
static void write_slot_then_read_slot_round_trip() {
    std::string path = tempPath("slot_roundtrip");
    std::filesystem::remove(path);

    CHK(writeSlot(path, 0, 256, "page zero payload") == Status::OK);

    std::string out;
    CHK(readSlot(path, 0, 256, out) == Status::OK);
    CHK(out == "page zero payload");

    std::filesystem::remove(path);
}

// Verifies multiple slots at different, non-adjacent indices in the same
// file don't clobber each other (exercises the offset = index * slotSize
// math this is meant to guarantee).
static void multiple_slots_do_not_clobber_each_other() {
    std::string path = tempPath("slot_multi");
    std::filesystem::remove(path);

    CHK(writeSlot(path, 0, 128, "first") == Status::OK);
    CHK(writeSlot(path, 5, 128, "sixth") == Status::OK);
    CHK(writeSlot(path, 2, 128, "third") == Status::OK);

    std::string out;
    CHK(readSlot(path, 0, 128, out) == Status::OK);
    CHK(out == "first");
    CHK(readSlot(path, 5, 128, out) == Status::OK);
    CHK(out == "sixth");
    CHK(readSlot(path, 2, 128, out) == Status::OK);
    CHK(out == "third");

    std::filesystem::remove(path);
}

// Verifies writeSlot rejects a payload that doesn't fit in the slot size,
// rather than silently truncating it.
static void write_slot_oversized_payload_returns_out_of_memory() {
    std::string path = tempPath("slot_oversized");
    std::filesystem::remove(path);

    std::string tooBig(300, 'x');
    CHK(writeSlot(path, 0, 256, tooBig) == Status::OUT_OF_MEMORY);
}

// Verifies readSlot on a slot that was never written (file doesn't exist
// yet) returns NOT_FOUND.
static void read_slot_missing_file_returns_not_found() {
    std::string path = tempPath("slot_missing_file");
    std::filesystem::remove(path);

    std::string out;
    CHK(readSlot(path, 0, 128, out) == Status::NOT_FOUND);
}

// Verifies readSlot on a slot within an existing file's allocated range,
// but never explicitly written, returns NOT_FOUND (zero length prefix)
// rather than returning garbage as a successful payload.
static void read_slot_never_written_returns_not_found() {
    std::string path = tempPath("slot_never_written");
    std::filesystem::remove(path);

    CHK(writeSlot(path, 5, 128, "only slot 5 written") == Status::OK);

    std::string out;
    CHK(readSlot(path, 2, 128, out) == Status::NOT_FOUND);

    std::filesystem::remove(path);
}

// Verifies readSlot detects a corrupt length prefix (larger than the slot
// can possibly hold) and returns PARSE_ERROR rather than reading out of
// bounds or returning garbage.
static void read_slot_corrupt_length_prefix_returns_parse_error() {
    std::string path = tempPath("slot_corrupt");
    std::filesystem::remove(path);

    // Hand-craft a slot with an invalid (too-large) length prefix.
    constexpr std::uint32_t slotSize = 64;
    std::vector<char> raw(slotSize, 0);
    std::uint32_t badLen = slotSize * 10; // clearly too large to fit
    std::memcpy(raw.data(), &badLen, sizeof(badLen));

    {
        std::ofstream f(path, std::ios::binary);
        f.write(raw.data(), static_cast<std::streamsize>(raw.size()));
    }

    std::string out;
    CHK(readSlot(path, 0, slotSize, out) == Status::PARSE_ERROR);

    std::filesystem::remove(path);
}

// Executes all FileIO test cases.
static void run_tests() {
    RUN(write_atomic_then_read_round_trip);
    RUN(write_atomic_overwrites_existing_content);
    RUN(write_atomic_leaves_no_tmp_file);
    RUN(read_missing_file_returns_io_error);
    RUN(write_slot_then_read_slot_round_trip);
    RUN(multiple_slots_do_not_clobber_each_other);
    RUN(write_slot_oversized_payload_returns_out_of_memory);
    RUN(read_slot_missing_file_returns_not_found);
    RUN(read_slot_never_written_returns_not_found);
    RUN(read_slot_corrupt_length_prefix_returns_parse_error);
}

REGISTER_TEST_SUITE();
