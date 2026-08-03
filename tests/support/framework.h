#pragma once

// Single include that pulls in the entire test framework — suite
// files include only this to get the library under test, testing
// macros, output helpers, and suite registration.

// clang-format off
#include <MiniDB/Core/Database.h>        // Database: named collection of Tables, save()/load() persistence
#include <MiniDB/Core/Page.h>            // Page: fixed-slot in-memory container of Records within a Table
#include <MiniDB/Core/Record.h>          // Record: schema-validated field container, the unit of storage
#include <MiniDB/Core/Table.h>           // Table: schema + CRUD + page management for one named table
#include <MiniDB/Common/FileIO.h>        // atomic file write, sized read, fixed-slot read/write primitives

#include <MiniDB/Engine/Concurrency.h>   // ThreadPool-backed parallel save/load/rebuildIndex/export across tables
#include <MiniDB/Engine/QueryEngine.h>   // filter/sort/limit/aggregate queries over a Table
#include <MiniDB/Engine/Serializer.h>    // table/database JSON export-import, independent of Database::save/load
#include <MiniDB/Engine/StorageEngine.h> // per-page disk I/O + LRU page cache, plus whole-database save/load passthrough

#include "helpers.h"                 // output formatting & printing helpers
#include "macros.h"                  // RUN/CHK/CHK_THROWS, REGISTER_TEST_SUITE()
#include "registry.h"                // TestSuite, TestRegistrar, test_registry()

#include <iomanip>                   // std::setw
// clang-format on

// ── Suite drivers ───────────────────────────────────────────────────
//
// These build on registry.h (test_registry()) and helpers.h (setTitle(),
// stats()) — kept here rather than in helpers.h so that dependency only
// flows one way: this file already includes both before any of the
// below is defined.

// Runs every registered suite in order, then prints the overall stats.
inline void printAllTestSuite() {
    for (const auto& suite : test_registry()) {
        std::cout << "\n";
        setTitle(suite.name);
        suite.run();
    }
    std::cout << "\n";
    stats();
    std::cout << "\n";
}

// Prints every registered suite's id and name, grouped by category
// header, without running any of them.
inline void printTestSuiteList() {
    std::cout << "\nAvailable test suites:\n";
    std::string category;

    for (const auto& suite : test_registry()) {
        if (category != suite.category)
            std::cout << '\n' << CYAN << prettify(suite.category) << RESET << '\n';
        std::cout << GREEN << std::left << std::setw(6) << ("[" + suite.id + "]") << RESET
                  << std::setw(30) << suite.name << '\n';
        category = suite.category;
    }

    std::cout << "\n";
}

// Runs a single suite and prints its title. Does not print stats —
// callers decide when to summarize, since a category match runs this
// once per suite but only wants stats printed once, at the end.
inline void printOneSuite(const TestSuite& suite) {
    std::cout << "\n";
    setTitle(suite.name);
    suite.run();
    std::cout << "\n";
}