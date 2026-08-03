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

#include <MiniDB/Common/FileIO.h>
#include <support/framework.h>

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
static void save_failure_file_untouched() {
    const std::string dbPath = tempDir("atomic_write") + "/db.json";

    Database original("orig");
    CHK(original.createTable("t", Vector<ColumnDef>{}) == Status::OK);
    CHK(original.save(dbPath) == Status::OK);

    std::string before;
    CHK(MiniDB::Common::FileIO::readFile(dbPath, before) == Status::OK);

    const std::string tmpPath = dbPath + ".tmp";
    std::filesystem::create_directories(tmpPath);

    Database changed("changed");
    CHK(changed.createTable("t2", Vector<ColumnDef>{}) == Status::OK);
    CHK(changed.save(dbPath) != Status::OK);

    std::string after;
    CHK(MiniDB::Common::FileIO::readFile(dbPath, after) == Status::OK);
    CHK(after == before);
}

// Verifies the failure above is transient, not a permanently broken state:
// once the obstruction is removed, save() succeeds and the new content is
// what actually lands on disk.
static void save_succeeds_after_recovery() {
    const std::string dbPath = tempDir("atomic_write_recovery") + "/db.json";

    Database original("orig");
    CHK(original.createTable("t", Vector<ColumnDef>{}) == Status::OK);
    CHK(original.save(dbPath) == Status::OK);

    const std::string tmpPath = dbPath + ".tmp";
    std::filesystem::create_directories(tmpPath);

    Database changed("changed");
    CHK(changed.createTable("t2", Vector<ColumnDef>{}) == Status::OK);
    CHK(changed.save(dbPath) != Status::OK);

    std::filesystem::remove_all(tmpPath);
    CHK(changed.save(dbPath) == Status::OK);

    std::string after;
    CHK(MiniDB::Common::FileIO::readFile(dbPath, after) == Status::OK);
    CHK(after == changed.toJson().dump());
}

static void run_tests() {
    RUN(save_failure_file_untouched);
    RUN(save_succeeds_after_recovery);
}

REGISTER_TEST_SUITE();