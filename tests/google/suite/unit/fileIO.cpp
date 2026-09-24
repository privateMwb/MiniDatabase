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

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

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
TEST(FileIO, WriteAtomicThenReadRoundTrip) {
    std::string path = tempPath("atomic_roundtrip");
    std::filesystem::remove(path);

    ASSERT_EQ(writeFileAtomic(path, "hello world"), Status::OK);

    std::string out;
    ASSERT_EQ(readFile(path, out), Status::OK);
    EXPECT_EQ(out, "hello world");

    std::filesystem::remove(path);
}

// Verifies writeFileAtomic overwrites existing content rather than
// appending or leaving stale bytes behind.
TEST(FileIO, WriteAtomicOverwritesExistingContent) {
    std::string path = tempPath("atomic_overwrite");
    std::filesystem::remove(path);

    ASSERT_EQ(writeFileAtomic(path, "aaaaaaaaaa"), Status::OK);
    ASSERT_EQ(writeFileAtomic(path, "bb"), Status::OK);

    std::string out;
    ASSERT_EQ(readFile(path, out), Status::OK);
    EXPECT_EQ(out, "bb");

    std::filesystem::remove(path);
}

// Verifies writeFileAtomic does not leave a stray ".tmp" file behind after
// a successful write.
TEST(FileIO, WriteAtomicLeavesNoTmpFile) {
    std::string path = tempPath("atomic_no_tmp");
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".tmp");

    ASSERT_EQ(writeFileAtomic(path, "content"), Status::OK);
    EXPECT_FALSE(std::filesystem::exists(path + ".tmp"));

    std::filesystem::remove(path);
}

// Verifies readFile on a nonexistent file returns IO_ERROR rather than
// crashing or returning an empty success.
TEST(FileIO, ReadMissingFileReturnsIoError) {
    std::string path = tempPath("does_not_exist");
    std::filesystem::remove(path);

    std::string out;
    EXPECT_EQ(readFile(path, out), Status::IO_ERROR);
}

// Verifies writeSlot followed by readSlot at the same index returns the
// same payload.
TEST(FileIO, WriteSlotThenReadSlotRoundTrip) {
    std::string path = tempPath("slot_roundtrip");
    std::filesystem::remove(path);

    ASSERT_EQ(writeSlot(path, 0, 256, "page zero payload"), Status::OK);

    std::string out;
    ASSERT_EQ(readSlot(path, 0, 256, out), Status::OK);
    EXPECT_EQ(out, "page zero payload");

    std::filesystem::remove(path);
}

// Verifies multiple slots at different, non-adjacent indices in the same
// file don't clobber each other (exercises the offset = index * slotSize
// math this is meant to guarantee).
TEST(FileIO, MultipleSlotsDoNotClobberEachOther) {
    std::string path = tempPath("slot_multi");
    std::filesystem::remove(path);

    ASSERT_EQ(writeSlot(path, 0, 128, "first"), Status::OK);
    ASSERT_EQ(writeSlot(path, 5, 128, "sixth"), Status::OK);
    ASSERT_EQ(writeSlot(path, 2, 128, "third"), Status::OK);

    std::string out;
    ASSERT_EQ(readSlot(path, 0, 128, out), Status::OK);
    EXPECT_EQ(out, "first");
    ASSERT_EQ(readSlot(path, 5, 128, out), Status::OK);
    EXPECT_EQ(out, "sixth");
    ASSERT_EQ(readSlot(path, 2, 128, out), Status::OK);
    EXPECT_EQ(out, "third");

    std::filesystem::remove(path);
}

// Verifies writeSlot rejects a payload that doesn't fit in the slot size,
// rather than silently truncating it.
TEST(FileIO, WriteSlotOversizedPayloadReturnsOutOfMemory) {
    std::string path = tempPath("slot_oversized");
    std::filesystem::remove(path);

    std::string tooBig(300, 'x');
    EXPECT_EQ(writeSlot(path, 0, 256, tooBig), Status::OUT_OF_MEMORY);
}

// Verifies readSlot on a slot that was never written (file doesn't exist
// yet) returns NOT_FOUND.
TEST(FileIO, ReadSlotMissingFileReturnsNotFound) {
    std::string path = tempPath("slot_missing_file");
    std::filesystem::remove(path);

    std::string out;
    EXPECT_EQ(readSlot(path, 0, 128, out), Status::NOT_FOUND);
}

// Verifies readSlot on a slot within an existing file's allocated range,
// but never explicitly written, returns NOT_FOUND (zero length prefix)
// rather than returning garbage as a successful payload.
TEST(FileIO, ReadSlotNeverWrittenReturnsNotFound) {
    std::string path = tempPath("slot_never_written");
    std::filesystem::remove(path);

    ASSERT_EQ(writeSlot(path, 5, 128, "only slot 5 written"), Status::OK);

    std::string out;
    EXPECT_EQ(readSlot(path, 2, 128, out), Status::NOT_FOUND);

    std::filesystem::remove(path);
}

// Verifies readSlot detects a corrupt length prefix (larger than the slot
// can possibly hold) and returns PARSE_ERROR rather than reading out of
// bounds or returning garbage.
TEST(FileIO, ReadSlotCorruptLengthPrefixReturnsParseError) {
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
    EXPECT_EQ(readSlot(path, 0, slotSize, out), Status::PARSE_ERROR);

    std::filesystem::remove(path);
}
