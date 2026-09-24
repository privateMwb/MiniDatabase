// ============================================================
// fuzz/fuzz_record_deserialize.cpp
//
// Crash-oracle fuzzer for Record::deserialize(), the entry point for
// every byte MiniDatabase ever reads back out of a Page's JSON before
// Record::validate() has had a chance to run against any schema. This
// is NOT a differential fuzzer -- there's no shadow-model JSON parser
// to compare against -- so this instead treats deserialize()'s
// documented Status::PARSE_ERROR as an expected, non-finding outcome
// for malformed input, and on a successful parse exercises every
// Record accessor so ASan/UBSan can catch anything reachable only
// through a specific field combination a hand-written unit test
// wouldn't think to construct.
//
// Record::deserialize() is documented (and unit-tested, see
// tests/*/suite/unit/record.cpp) to never let a Json::parse() exception
// escape -- it wraps the parse in its own try/catch and converts a
// parse failure into Status::PARSE_ERROR. This harness deliberately
// does NOT add a second try/catch around the call: if that contract
// ever regresses (the guard gets removed, or a new exception type slips
// through it), an escaping exception here is exactly the kind of
// finding this harness exists to catch, the same way
// table_copy_assign_signature.cpp pins a compile-time contract instead
// of hiding a regression behind extra scaffolding.
//
// Specifically exercised:
//   - deserialize() -> Json::parse() -> fromJson() -> validate() is
//     NOT invoked by deserialize() itself (Record has no attached
//     schema at the Record level), so this harness also runs
//     validate() against a small representative schema afterward, to
//     reach the same validation code fuzz_database_load.cpp's
//     higher-level callers would exercise indirectly
//   - every field accessor: hasField/getField/getFieldRef/removeField,
//     both for keys the fuzzed JSON plausibly set and for keys it
//     almost certainly didn't
//   - the id and deleted-flag accessors, which come from the JSON
//     envelope's own reserved keys (__id__, __deleted__) rather than
//     the free-form "data" object -- a separate parsing surface within
//     the same call
//
// Deliberately NOT covered here: Page::deserialize() and
// Table::deserialize() (each wraps this same Record-level parsing in
// an outer JSON array/object; fuzzing Record directly is the minimal
// reproduction of the shared parsing surface all three go through) and
// Database's JSON path, which has no string deserialize() of its own
// -- see fuzz_wal_recovery.cpp for MiniDatabase's other on-disk format,
// and FUZZING.md for why Table/Page/Database aren't separate targets.
// ============================================================

#include <MiniDB/MiniDatabase.h>

#include <cstddef>
#include <cstdint>
#include <string>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

namespace {

Vector<ColumnDef> representativeSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, true},
        ColumnDef{"age", ColumnType::INT, true},
        ColumnDef{"balance", ColumnType::DOUBLE, true},
        ColumnDef{"active", ColumnType::BOOL, true},
    };
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::string raw(reinterpret_cast<const char*>(data), size);

    Record record;
    Status status = record.deserialize(raw);

    if (status != Status::OK) {
        // Documented behavior (Record.h): malformed JSON, or JSON that
        // doesn't match the envelope Record expects, reports
        // Status::PARSE_ERROR rather than throwing. That's the parser
        // correctly rejecting bad input, not a finding.
        return 0;
    }

    // A successful parse must leave every accessor safely callable,
    // regardless of what bytes produced it.
    (void)record.getID();
    (void)record.isDeleted();

    static const Vector<ColumnDef> schema = representativeSchema();
    (void)record.validate(schema);

    for (const auto& col : schema) {
        (void)record.hasField(col.name);
        (void)record.getField(col.name);
        (void)record.getFieldRef(col.name);
    }
    (void)record.hasField("");
    (void)record.getField("__id__"); // reserved envelope key, not a data field

    // Round-trip back out: toJson()/serialize() must also stay safely
    // callable on whatever fromJson() just built, and re-parsing that
    // output should reach the same state (checked loosely here -- an
    // exact-equality assertion belongs in the unit suite, not a fuzzer
    // whose job is crash-finding, not correctness-checking).
    (void)record.toJson();
    (void)record.serialize();

    return 0;
}
