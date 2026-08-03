#pragma once

// Single include that pulls in the entire example framework — suite
// files include only this to get the library under test, example
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
#include "macros.h"                  // REGISTER_EXAMPLE_SUITE()
#include "registry.h"                // ExampleSuite, ExampleRegistrar, example_registry()

#include <iomanip>                   // std::setw
// clang-format on

// ── Suite drivers ───────────────────────────────────────────────────
//
// These build on registry.h (example_registry()) and helpers.h
// (mainTitle(), setTitle()) — kept here rather than in helpers.h so
// that dependency only flows one way: this file already includes both
// before any of the below is defined.

// Prints the top-level banner once, then runs every registered suite
// in order, each under its own section title.
inline void printAllExampleSuite() {
    std::cout << "\n";

    for (const auto& suite : example_registry()) {
        std::cout << "\n";
        mainTitle(suite.name);
        suite.run();
    }

    std::cout << "\n";
}

// Prints every registered suite's id and name, grouped by category
// header, without running any of them.
inline void printExampleSuiteList() {
    std::cout << "\nAvailable example suites:\n";
    std::string category;

    for (const auto& suite : example_registry()) {
        if (category != suite.category)
            std::cout << '\n' << CYAN << prettify(suite.category) << RESET << '\n';
        std::cout << GREEN << std::left << std::setw(6) << ("[" + suite.id + "]") << RESET
                  << std::setw(30) << suite.name << '\n';
        category = suite.category;
    }

    std::cout << "\n";
}

// Runs a single suite under its own section title. No top-level
// banner — callers running a scoped subset don't need the "whole run"
// framing that printAllExampleSuite() prints.
inline void printOneSuite(const ExampleSuite& suite) {
    std::cout << "\n";
    mainTitle(suite.name);
    suite.run();
    std::cout << "\n";
}