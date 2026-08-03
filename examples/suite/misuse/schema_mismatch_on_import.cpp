// Schema Mismatch On Import.
//
// Demonstrates:
// - Serializer's import functions insert into a Table you already created
//   -- they don't create the table or infer a schema for you
// - importing data whose fields don't match the target table's schema
//   fails per-record with the same Status::INVALID_TYPE / INVALID_SCHEMA
//   that insertRecord() would produce
// - a partially-matching import stops at the first bad record: earlier
//   records in the same file may already have been inserted before the
//   failure is reported

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;
using namespace MiniDB::Engine;

static void run_examples() {
    // Exported data has no schema attached to it -- it's just a JSON array
    // of records. Whatever schema the *target* table happens to have is
    // what every incoming record gets validated against.
    setTitle("Export Carries No Schema Of Its Own");

    Record ada(1);
    (void)ada.setField("name", Json("Ada"));
    Table users("users", 1, Vector<ColumnDef>{ColumnDef{"name", ColumnType::STRING, false}});
    (void)users.insertRecord(ada);

    std::string exported = Serializer::exportTableToJson(users);
    std::cout << "exported JSON : " << exported << "\n\n";

    // The misuse: importing that same JSON into a table with a
    // DIFFERENT, incompatible schema fails -- the import doesn't adapt to
    // the data, the data has to match the target.
    setTitle("The Misuse: Importing Into A Mismatched Schema");

    Table wrongSchema(
        "users", 2,
        Vector<ColumnDef>{
            ColumnDef{"name", ColumnType::INT, false}, // INT here, but the data is a string
        });

    Status s = Serializer::importTableFromJson(wrongSchema, exported);
    std::cout << "importTableFromJson (schema expects INT, data is STRING) : "
              << static_cast<int>(s) << " (INVALID_TYPE)\n";
    std::cout << "wrongSchema.recordCount()                                : "
              << wrongSchema.recordCount() << " (0 -- the one record failed validation)\n\n";

    // A sharper version of the same trap: with multiple records, the
    // import stops at the FIRST failure. Records before it may already be
    // inserted; records after it are never attempted.
    setTitle("Partial Import Stops At The First Bad Record");

    Table target("orders", 3,
                 Vector<ColumnDef>{
                     ColumnDef{"amount", ColumnType::INT, false},
                 });

    // Record 1: valid. Record 2: "amount" is a string, not an INT. Record
    // 3: would be valid too, but is never reached.
    std::string threeRecords =
        R"([{"id":1,"amount":10},{"id":2,"amount":"oops"},{"id":3,"amount":30}])";

    s = Serializer::importTableFromJson(target, threeRecords);
    std::cout << "importTableFromJson status : " << static_cast<int>(s) << " (INVALID_TYPE)\n";
    std::cout << "target.recordCount()       : " << target.recordCount()
              << " (1 -- only record 1 was inserted before the failure)\n";

    Record out;
    std::cout << "record 1 present  : " << static_cast<int>(target.getRecord(1, out)) << " (OK)\n";
    std::cout << "record 3 present  : " << static_cast<int>(target.getRecord(3, out))
              << " (NOT_FOUND -- import never got that far)\n";
}

REGISTER_EXAMPLE_SUITE();
