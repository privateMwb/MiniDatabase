// Basic CRUD.
//
// Demonstrates:
// - insertRecord, getRecord, updateRecord, deleteRecord on a single Table
// - the DUPLICATE_KEY / NOT_FOUND statuses each operation can return
// - that deleteRecord is logical (the record is marked deleted, not
//   physically erased until compact())

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static Vector<ColumnDef> userSchema() {
    return Vector<ColumnDef>{
        ColumnDef{"name", ColumnType::STRING, false},
        ColumnDef{"age", ColumnType::INT, false},
    };
}

static void run_examples() {
    Table users("users", 1, userSchema());

    // Create: insertRecord validates against the schema and rejects a
    // second record with the same id.
    setTitle("Insert");

    Record ada(1);
    (void)ada.setField("name", Json("Ada"));
    (void)ada.setField("age", Json(30));
    std::cout << "insert Ada (id 1) : " << static_cast<int>(users.insertRecord(ada)) << "\n";

    Record duplicate(1);
    (void)duplicate.setField("name", Json("Someone Else"));
    (void)duplicate.setField("age", Json(99));
    std::cout << "insert again (id 1, should fail) : "
              << static_cast<int>(users.insertRecord(duplicate)) << "\n\n";

    // Read: getRecord copies the matching record into `out`, or reports
    // NOT_FOUND for a missing id.
    setTitle("Read");

    Record out;
    std::cout << "getRecord(1) : " << static_cast<int>(users.getRecord(1, out)) << "\n";
    std::cout << "  name : " << out.getField("name").asString() << "\n";
    std::cout << "  age  : " << out.getField("age").asNumber() << "\n";
    std::cout << "getRecord(999), missing : " << static_cast<int>(users.getRecord(999, out))
              << "\n\n";

    // Update: replaces the stored record's data; the id must already exist.
    setTitle("Update");

    Record updated(1);
    (void)updated.setField("name", Json("Ada Lovelace"));
    (void)updated.setField("age", Json(31));
    std::cout << "updateRecord(1) : " << static_cast<int>(users.updateRecord(updated)) << "\n";

    (void)users.getRecord(1, out);
    std::cout << "  name after update : " << out.getField("name").asString() << "\n\n";

    // Delete: marks the record deleted. It stops showing up in getRecord
    // immediately, but its storage isn't reclaimed until compact() runs.
    setTitle("Delete");

    std::cout << "recordCount before delete : " << users.recordCount() << "\n";
    std::cout << "deleteRecord(1)           : " << static_cast<int>(users.deleteRecord(1)) << "\n";
    std::cout << "recordCount after delete  : " << users.recordCount() << "\n";
    std::cout << "getRecord(1) after delete : " << static_cast<int>(users.getRecord(1, out))
              << "\n";
    std::cout << "deleteRecord(1) again     : " << static_cast<int>(users.deleteRecord(1)) << "\n";
}

REGISTER_EXAMPLE_SUITE();
