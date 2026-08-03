// Duplicate Record IDs.
//
// Demonstrates:
// - insertRecord() fails with DUPLICATE_KEY if the id already exists --
//   it never silently overwrites
// - the fix: use updateRecord() when you mean to replace an existing
//   record, or check getRecord() first if you're not sure which one you
//   need
// - a common trap: reusing an id counter (e.g. starting back at 0) after
//   deleting records, when the original ids are still present elsewhere

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static Vector<ColumnDef> orderSchema() {
    return Vector<ColumnDef>{ColumnDef{"item", ColumnType::STRING, false}};
}

static void run_examples() {
    Table orders("orders", 1, orderSchema());

    Record first(1);
    (void)first.setField("item", Json("widget"));
    (void)orders.insertRecord(first);

    // The misuse: inserting a second record with the same id doesn't
    // overwrite the first one -- it's rejected outright.
    setTitle("The Misuse");

    Record second(1); // same id as `first`
    (void)second.setField("item", Json("gadget"));

    Status s = orders.insertRecord(second);
    std::cout << "insertRecord with a reused id : " << static_cast<int>(s) << " (DUPLICATE_KEY)\n";

    Record stillOriginal;
    (void)orders.getRecord(1, stillOriginal);
    std::cout << "record 1 is still            : " << stillOriginal.getField("item").asString()
              << " (the original insert, untouched)\n\n";

    // The fix: if you actually mean to replace it, call updateRecord().
    setTitle("Fix: updateRecord() To Replace");

    s = orders.updateRecord(second);
    std::cout << "updateRecord with the same id : " << static_cast<int>(s) << " (OK)\n";

    Record afterUpdate;
    (void)orders.getRecord(1, afterUpdate);
    std::cout << "record 1 is now               : " << afterUpdate.getField("item").asString()
              << "\n\n";

    // A related trap: manually tracking "the next id to use" and resetting
    // it after deletes can produce an id that collides with a record that
    // is still very much alive elsewhere in the table.
    setTitle("A Related Trap: Manual ID Counters");

    Record third(2);
    (void)third.setField("item", Json("thingamajig"));
    (void)orders.insertRecord(third);

    (void)orders.deleteRecord(1); // frees up "slot 1" logically, but...

    Record fourth(1); // reusing id 1 is fine now -- 1 was deleted...
    (void)fourth.setField("item", Json("new item at id 1"));
    s = orders.insertRecord(fourth);
    std::cout << "reinserting at a deleted id (1) : " << static_cast<int>(s)
              << " (OK -- 1 was freed)\n";

    Record fifth(2); // ...but id 2 is still in use! this is NOT freed.
    (void)fifth.setField("item", Json("collides with third"));
    s = orders.insertRecord(fifth);
    std::cout << "reinserting at a LIVE id (2)    : " << static_cast<int>(s)
              << " (DUPLICATE_KEY -- 2 was never deleted)\n";
}

REGISTER_EXAMPLE_SUITE();
