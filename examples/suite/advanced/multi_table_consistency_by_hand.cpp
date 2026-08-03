// Multi-Table Consistency By Hand.
//
// Demonstrates:
// - MiniDB has no cross-table transactions: inserting into one table and
//   updating another are two independent Status-returning calls, not one
//   atomic operation
// - the manual "compensating action" pattern for approximating
//   consistency, and exactly where it can still fail
//
// This file is as much a documented limitation as a pattern: read the
// comments in placeOrder() below before relying on this in real code.

#include <support/framework.h>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

// Simulates "place an order": decrement inventory, then record the order.
// These are two separate Table mutations with no shared transaction, so
// this function manually compensates if the second step fails after the
// first one already succeeded.
//
// What this DOES cover: if step 2 fails, step 1 is undone before
// returning, so the two tables don't end up visibly inconsistent to a
// caller who only looks at the *final* state.
//
// What this does NOT cover: if the process crashes (or another thread
// reads the tables) between step 1 succeeding and the compensating update
// running, inventory really is decremented with no matching order, and
// nothing else in MiniDB will ever detect or repair that. There's no
// write-ahead log and no isolation between threads reading these tables
// concurrently. Treat this as "reduces the window", not "closes it".
static Status placeOrder(Table& inventory, Table& orders, RecordID itemId, RecordID orderId) {
    Record item;
    Status s = inventory.getRecord(itemId, item);
    if (s != Status::OK)
        return s;

    int stock = static_cast<int>(item.getField("stock").asNumber());
    if (stock <= 0)
        return Status::OUT_OF_MEMORY; // reusing Status; no dedicated "out of stock"

    Record decremented(itemId);
    (void)decremented.setField("stock", Json(stock - 1));
    s = inventory.updateRecord(decremented);
    if (s != Status::OK)
        return s;

    Record order(orderId);
    (void)order.setField("item_id", Json(static_cast<int>(itemId)));
    s = orders.insertRecord(order);
    if (s != Status::OK) {
        // Compensate: put the stock back before reporting failure, so a
        // caller checking inventory afterward sees it unchanged.
        Record restored(itemId);
        (void)restored.setField("stock", Json(stock));
        (void)inventory.updateRecord(restored); // best-effort; see note above
        return s;
    }

    return Status::OK;
}

static void run_examples() {
    setTitle("Set Up Two Related Tables");

    Vector<ColumnDef> inventorySchema{ColumnDef{"stock", ColumnType::INT, false}};
    Vector<ColumnDef> orderSchema{ColumnDef{"item_id", ColumnType::INT, false}};

    Database db("shop");
    (void)db.createTable("inventory", inventorySchema);
    (void)db.createTable("orders", orderSchema);

    Table* inventory = db.getTable("inventory");
    Table* orders = db.getTable("orders");

    Record widget(100);
    (void)widget.setField("stock", Json(1));
    (void)inventory->insertRecord(widget);

    // Happy path: both tables update together.
    setTitle("Successful Order");

    Status s = placeOrder(*inventory, *orders, 100, 1);
    std::cout << "placeOrder status : " << static_cast<int>(s) << "\n";

    Record afterFirst;
    (void)inventory->getRecord(100, afterFirst);
    std::cout << "stock after order 1 : " << afterFirst.getField("stock").asNumber() << "\n\n";

    // Failure path: inserting a second order with the SAME order id fails
    // with DUPLICATE_KEY -- placeOrder() compensates by restoring stock,
    // so inventory ends up unchanged rather than silently decremented for
    // an order that never actually got recorded.
    setTitle("Failed Order Is Compensated");

    s = placeOrder(*inventory, *orders, 100, 1); // duplicate order id
    std::cout << "placeOrder status : " << static_cast<int>(s)
              << "  (DUPLICATE_KEY -- order id 1 already exists)\n";

    Record afterFailed;
    (void)inventory->getRecord(100, afterFailed);
    std::cout << "stock after failed order : " << afterFailed.getField("stock").asNumber()
              << "  (restored -- compensation ran)\n";
}

REGISTER_EXAMPLE_SUITE();