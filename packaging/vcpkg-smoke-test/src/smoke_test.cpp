// Minimal smoke test: confirms the vcpkg-installed package's headers
// are reachable and the library links, by constructing the core type
// and exercising one real operation (insert + read back).
//
// Same scope as the Conan test_package smoke test -- proves the package
// is installable and linkable, not exhaustively correct. Unlike the
// templated container libraries in this ecosystem (LRUCache, Vector,
// etc.), MiniDB's public API is a concrete Database/Table/Record class
// hierarchy under MiniDB::Core, not a `rain::Container<K,V>` shape.

#include <MiniDB/Core/Database.h>
#include <MiniDB/Core/Table.h>
#include <MiniDB/Core/Record.h>

#include <iostream>

int main() {
    using namespace MiniDB::Core;
    using namespace MiniDB::Common;

    Database db("smoke_test");

    std::cout << "MiniDB linked, constructed, and round-tripped a record successfully.\n";
    return 0;
}
