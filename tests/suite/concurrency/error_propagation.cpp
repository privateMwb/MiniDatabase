// Concurrency Error Propagation Test Suite
// Pins runParallel()'s fold behavior (Concurrency.cpp): results are
// collected in the SAME order as Database::getTables() (table creation
// order), not completion order, so "last non-OK wins" is fully
// deterministic -- it means "the last table in creation order that
// failed", not "whichever failure happened to finish last on the pool".
// This suite also confirms one table's failure does NOT roll back another
// table's successful result -- unlike Database::fromJson's atomic
// scratch-then-swap, this facility applies each table's outcome
// independently.
//
// Covers:
// - all-success returns Status::OK
// - the fold picks the LAST non-OK result in creation order, not the
//   first and not the "worst"
// - one table's failure doesn't roll back another table's success
// - the same fold/partial-apply behavior holds for saveAllTablesParallel

#include <support/framework.h>

#include <filesystem>
#include <fstream>

using namespace MiniDB::Core;
using namespace MiniDB::Engine;
using namespace MiniDB::Common;

namespace {
std::string tempDir(const std::string& label) {
    auto dir = std::filesystem::temp_directory_path() / ("minidb_concurrency_" + label);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}

void writeRawFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f << content;
}
} // namespace

// Baseline: when every table's load succeeds, the fold returns
// Status::OK -- confirms the "last non-OK wins" fold doesn't somehow
// stay non-OK when nothing actually failed.
static void all_succeed_returns_ok() {
    const std::string dir = tempDir("error_baseline");

    Database db("shop");
    CHK(db.createTable("a", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("b", Vector<ColumnDef>{}) == Status::OK);
    writeRawFile(dir + "/db_a.json", "[]");
    writeRawFile(dir + "/db_b.json", "[]");

    MiniDB::Engine::Concurrency conc;
    CHK(conc.loadAllTablesParallel(db, dir + "/db") == Status::OK);
}

// Pins the specific fold order: table "a" (created first) fails with
// Status::IO_ERROR (its file is simply never created); table "b" (created
// second) fails with Status::INVALID_TYPE (its file contains a record
// whose field violates b's schema); table "c" (created third) succeeds.
// Since "b" is the LAST non-OK result in creation/collection order, the
// aggregate status must be INVALID_TYPE, not IO_ERROR -- proving the fold
// is "last wins", not "first wins" or "worst wins".
static void last_non_ok_status_wins() {
    const std::string dir = tempDir("error_last_wins");
    const std::string base = dir + "/db";

    Database db("shop");
    CHK(db.createTable("a", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("b", Vector<ColumnDef>{ColumnDef{"n", ColumnType::INT, false}}) ==
        Status::OK);
    CHK(db.createTable("c", Vector<ColumnDef>{}) == Status::OK);

    // "a": no file at all -> FileIO::readFile fails -> Status::IO_ERROR.
    // (intentionally not created)

    // "b": valid JSON, but the "n" field is a string where the schema
    // requires INT -> Record::validate() -> Status::INVALID_TYPE.
    writeRawFile(dir + "/db_b.json", R"([{"n":"not-a-number"}])");

    // "c": succeeds trivially.
    writeRawFile(dir + "/db_c.json", "[]");

    MiniDB::Engine::Concurrency conc;
    Status result = conc.loadAllTablesParallel(db, base);

    CHK(result == Status::INVALID_TYPE);
    CHK(result != Status::IO_ERROR);
}

// Verifies that "b"'s failure does not prevent "c"'s successful load from
// being applied -- the aggregate Status can be non-OK while a different
// table in the same batch still ends up fully, correctly loaded.
static void failure_partial_apply() {
    const std::string dir = tempDir("error_partial_apply");
    const std::string base = dir + "/db";

    Database db("shop");
    CHK(db.createTable("b", Vector<ColumnDef>{ColumnDef{"n", ColumnType::INT, false}}) ==
        Status::OK);
    CHK(db.createTable("c", Vector<ColumnDef>{}) == Status::OK);

    writeRawFile(dir + "/db_b.json", R"([{"n":"not-a-number"}])");
    writeRawFile(dir + "/db_c.json", R"([{"id":1},{"id":2},{"id":3}])");

    MiniDB::Engine::Concurrency conc;
    Status result = conc.loadAllTablesParallel(db, base);

    CHK(result != Status::OK);
    CHK(db.getTable("b")->recordCount() == 0); // b's insert never applied
    CHK(db.getTable("c")->recordCount() == 3); // c succeeded independently
}

// Verifies the same fold logic applies to saveAllTablesParallel(), not
// just loadAllTablesParallel() -- both share runParallel() but are
// separate call sites, so this isn't guaranteed by the earlier tests.
// NOTE: both obstructed tables fail with the same Status::IO_ERROR here
// (writeFileAtomic has only one failure code), so unlike
// last_non_ok_status_wins() this can't distinguish "last" from "first" --
// it only confirms the fold and partial-success behavior generalize to save.
static void save_shares_fold() {
    const std::string dir = tempDir("error_save_last_wins");
    const std::string base = dir + "/db";

    Database db("shop");
    CHK(db.createTable("a", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("b", Vector<ColumnDef>{}) == Status::OK);
    CHK(db.createTable("c", Vector<ColumnDef>{}) == Status::OK);

    std::filesystem::create_directories(base + "_a.json.tmp");
    std::filesystem::create_directories(base + "_b.json.tmp");

    MiniDB::Engine::Concurrency conc;
    Status result = conc.saveAllTablesParallel(db, base);

    CHK(result == Status::IO_ERROR);
    CHK(std::filesystem::exists(base + "_c.json")); // c's success still applied despite a/b failing
}

static void run_tests() {
    RUN(all_succeed_returns_ok);
    RUN(last_non_ok_status_wins);
    RUN(failure_partial_apply);
    RUN(save_shares_fold);
}

REGISTER_TEST_SUITE();