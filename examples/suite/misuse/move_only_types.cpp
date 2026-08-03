// Move-Only Types.
//
// Demonstrates:
// - Page, Table, and Database cannot be copied -- only moved
// - the compile error you'd hit trying to copy one (shown as a comment,
//   since it must not actually compile)
// - the fix: pass by reference/pointer when you don't need ownership, or
//   std::move() when you do mean to transfer it
// - what happens to the source of a move (it's left empty/invalid, not
//   just "duplicated for free")

#include <support/framework.h>

#include <type_traits>

using namespace MiniDB::Core;
using namespace MiniDB::Common;

static void run_examples() {
    // Why: Table owns a Vector<Page*> of heap-allocated pages (and each
    // Page owns a pool-backed array of records). A copy would either have
    // to deep-copy all of that on every pass-by-value, or shallow-copy the
    // pointers and risk two Tables freeing the same memory. Disabling copy
    // avoids both problems -- you always know exactly one Table owns a
    // given page.
    setTitle("Why Copy Is Disabled");

    std::cout << "Table is copy-constructible : "
              << std::is_copy_constructible_v<Table> << " (false)\n";
    std::cout << "Table is move-constructible : "
              << std::is_move_constructible_v<Table> << " (true)\n\n";

    // The misuse: code that "just works" for most types quietly fails to
    // compile for Table/Page/Database. This is deliberate -- it's caught at
    // compile time, not as a runtime surprise.
    setTitle("The Misuse (Does Not Compile)");

    std::cout << "// Table copy(original);           // ERROR: copy constructor is deleted\n";
    std::cout << "// Table another = original;        // ERROR: same reason\n";
    std::cout << "// std::vector<Table> tables;\n";
    std::cout
        << "// tables.push_back(original);      // ERROR: push_back(const T&) needs a copy\n\n";

    // The fix, option 1: don't take ownership at all -- use a reference or
    // pointer if the function just needs to read or mutate an existing
    // Table someone else owns.
    setTitle("Fix 1: Pass By Reference");

    Table original("orders", 1, Vector<ColumnDef>{});
    (void)original.insertRecord(Record(1));

    auto describe = [](const Table& t) {
        std::cout << "describe(): " << t.getName() << " has " << t.recordCount() << " record(s)\n";
    };
    describe(original);
    std::cout << "\n";

    // The fix, option 2: when you do mean to transfer ownership (e.g.
    // moving a Table into a container, or returning one from a factory
    // function), use std::move() explicitly.
    setTitle("Fix 2: std::move() When You Mean To Transfer");

    Table moved(std::move(original));
    std::cout << "moved.getName()      : " << moved.getName() << "\n";
    std::cout << "moved.recordCount()  : " << moved.recordCount() << "\n";

    // The source of a move is left in a valid but empty state -- it's not
    // an error to keep using it, but it no longer holds what it used to.
    std::cout << "original.getID()     : " << original.getID()
              << " (INVALID_TABLE_ID -- the pages were transferred, not copied)\n";
    std::cout << "original.isEmpty()   : " << original.isEmpty() << " (true)\n";
}

REGISTER_EXAMPLE_SUITE();
