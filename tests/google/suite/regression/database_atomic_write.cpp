// Database Atomic Write Regression Test
// Pins Database::save's atomic-write guarantee: write to a sibling .tmp
// file, then rename over the destination. A failure before the rename
// must leave the existing on-disk file untouched, and the system must
// recover cleanly once the obstruction is gone.
//
// Covers:
// - a blocked .tmp write (simulated by pre-creating the .tmp path as a
//   directory) leaves the previously-saved file byte-for-byte unchanged
// - the failure is transient: once the obstruction is removed, save()
//   succeeds and the new content is exactly what lands on disk

#include <MiniDB/MiniDatabase.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {
std::string tempDir(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path() / ("minidb_regression_" + label);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}
} // namespace

// Verifies that if the .tmp file can't be written (simulated by
// pre-creating the ".tmp" path as a directory), a subsequent save() leaves
// the previously-saved file byte-for-byte unchanged.
TEST(DatabaseAtomicWrite, SaveFailureFileUntouched) {
    const std::string dbPath = tempDir("atomic_write") + "/db.json";

    Database original("orig");
    ASSERT_EQ(original.createTable("t", Vector<ColumnDef>{}), Status::OK);
    ASSERT_EQ(original.save(dbPath), Status::OK);

    std::string before;
    ASSERT_EQ(MiniDB::Common::FileIO::readFile(dbPath, before), Status::OK);

    const std::string tmpPath = dbPath + ".tmp";
    std::filesystem::create_directories(tmpPath);

    Database changed("changed");
    ASSERT_EQ(changed.createTable("t2", Vector<ColumnDef>{}), Status::OK);
    ASSERT_NE(changed.save(dbPath), Status::OK);

    std::string after;
    ASSERT_EQ(MiniDB::Common::FileIO::readFile(dbPath, after), Status::OK);
    EXPECT_EQ(after, before);
}

// Verifies the failure above is transient, not a permanently broken state:
// once the obstruction is removed, save() succeeds and the new content is
// what actually lands on disk.
TEST(DatabaseAtomicWrite, SaveSucceedsAfterRecovery) {
    const std::string dbPath = tempDir("atomic_write_recovery") + "/db.json";

    Database original("orig");
    ASSERT_EQ(original.createTable("t", Vector<ColumnDef>{}), Status::OK);
    ASSERT_EQ(original.save(dbPath), Status::OK);

    const std::string tmpPath = dbPath + ".tmp";
    std::filesystem::create_directories(tmpPath);

    Database changed("changed");
    ASSERT_EQ(changed.createTable("t2", Vector<ColumnDef>{}), Status::OK);
    ASSERT_NE(changed.save(dbPath), Status::OK);

    std::filesystem::remove_all(tmpPath);
    ASSERT_EQ(changed.save(dbPath), Status::OK);

    std::string after;
    ASSERT_EQ(MiniDB::Common::FileIO::readFile(dbPath, after), Status::OK);
    EXPECT_EQ(after, changed.toJson().dump());
}
