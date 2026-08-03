// Hello, MiniDB.
//
// Demonstrates:
// - creating a Database
// - creating a Table with a schema
// - inserting a single Record
// - reading it back with getRecord

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static void run_examples() {
    // A Database is a named collection of Tables. It lives entirely in
    // memory until you explicitly save() it (see save_and_load.cpp).
    setTitle("Create a Database");

    Database db("greetings");
    std::cout << "Database name : " << db.getName() << "\n\n";

    // Every Table has a fixed schema: a list of columns, each with a type
    // and whether it's allowed to be missing (nullable).
    setTitle("Create a Table");

    Vector<ColumnDef> schema{
        ColumnDef{"message", ColumnType::STRING, false},
    };
    Status s = db.createTable("messages", schema);
    std::cout << "createTable status : " << static_cast<int>(s) << "\n\n";

    // Build a Record and set its fields before inserting it. Every mutating
    // call returns a Status -- checking it is not optional decoration, it's
    // how MiniDB reports failure (see pitfall/ and patterns/status_handling
    // for more on this).
    setTitle("Insert a Record");

    Table* messages = db.getTable("messages");
    Record hello(1);
    (void)hello.setField("message", Json("Hello, MiniDB!"));

    s = messages->insertRecord(hello);
    std::cout << "insertRecord status : " << static_cast<int>(s) << "\n\n";

    // Read it back by id.
    setTitle("Read It Back");

    Record out;
    s = messages->getRecord(1, out);
    std::cout << "getRecord status : " << static_cast<int>(s) << "\n";
    std::cout << "message          : " << out.getField("message").asString() << "\n";
}

REGISTER_EXAMPLE_SUITE();
