// Schema Design.
//
// Demonstrates:
// - defining a schema as a Vector<ColumnDef>
// - the four ColumnType kinds and how validate() checks each one
// - nullable vs required columns, and what happens when a required column
//   is missing
// - that INT specifically requires a whole number (no fractional part),
//   distinct from DOUBLE which accepts either

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static void run_examples() {
    // A schema is just a list of columns: name, type, and whether the
    // column is allowed to be absent from a given record.
    setTitle("Defining a Schema");

    Vector<ColumnDef> schema{
        ColumnDef{"title", ColumnType::STRING, false},  // required
        ColumnDef{"quantity", ColumnType::INT, false},  // required
        ColumnDef{"price", ColumnType::DOUBLE, false},  // required
        ColumnDef{"in_stock", ColumnType::BOOL, false}, // required
        ColumnDef{"note", ColumnType::STRING, true},    // optional
    };
    std::cout << "Column count : " << schema.size() << "\n\n";

    // A record with every required column present, correctly typed.
    setTitle("A Valid Record");

    Record product(1);
    (void)product.setField("title", Json("Widget"));
    (void)product.setField("quantity", Json(10));
    (void)product.setField("price", Json(9.99));
    (void)product.setField("in_stock", Json(true));
    // "note" is nullable, so it's fine to leave it out entirely.

    std::cout << "validate() : " << static_cast<int>(product.validate(schema)) << " (OK)\n\n";

    // Missing a required column fails validation; missing an optional one
    // does not.
    setTitle("Required vs Nullable");

    Record missingRequired(2);
    (void)missingRequired.setField("title", Json("Gadget"));
    // "quantity", "price", "in_stock" are all required and left unset.
    std::cout << "missing required columns : " << static_cast<int>(missingRequired.validate(schema))
              << " (INVALID_SCHEMA)\n";

    Record missingOptional(3);
    (void)missingOptional.setField("title", Json("Gizmo"));
    (void)missingOptional.setField("quantity", Json(5));
    (void)missingOptional.setField("price", Json(4.50));
    (void)missingOptional.setField("in_stock", Json(false));
    // "note" omitted -- this is fine, it's nullable.
    std::cout << "missing optional column   : "
              << static_cast<int>(missingOptional.validate(schema)) << " (OK)\n\n";

    // INT requires a whole number; DOUBLE accepts a fractional value.
    // A value of exactly 0 is a valid INT -- it is not treated as "empty".
    setTitle("INT vs DOUBLE");

    Record zero(4);
    (void)zero.setField("title", Json("Zero Quantity"));
    (void)zero.setField("quantity", Json(0)); // valid INT
    (void)zero.setField("price", Json(0.0));  // valid DOUBLE
    (void)zero.setField("in_stock", Json(false));
    std::cout << "quantity = 0 (INT)       : " << static_cast<int>(zero.validate(schema))
              << " (OK)\n";

    Record fractional(5);
    (void)fractional.setField("title", Json("Half a Widget"));
    (void)fractional.setField("quantity", Json(1.5)); // NOT a whole number
    (void)fractional.setField("price", Json(9.99));
    (void)fractional.setField("in_stock", Json(true));
    std::cout << "quantity = 1.5 (INT)     : " << static_cast<int>(fractional.validate(schema))
              << " (INVALID_TYPE)\n";
}

REGISTER_EXAMPLE_SUITE();
