#pragma once

// Single include that pulls in the entire benchmark framework — suite
// files include only this to get the library under test, benchmarking
// macros, output helpers, suite registration, and result export.

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

#include "export.h"                      // exportJson(), exportMarkdown()
#include "helpers.h"                     // output formatting & printing helpers
#include "macros.h"                      // BENCH/BENCH_SOLO/BENCH_CUSTOM, REGISTER_BENCH_SUITE()
#include "registry.h"                    // BenchSuite, BenchRegistrar, bench_registry()
#include "reference.h"                   // setCustom(), setStandard(); reference-implementation wrapper this suite benchmarks against
// clang-format on

// ── Project configuration ───────────────────────────────────────────

// The one function to edit when retargeting this skeleton at a
// different library: sets the display label for the custom
// implementation under test and the reference implementation it's
// compared against. Called once at startup, before any suite runs.
inline void setProjectLabels() {
    setCustom("MiniDB");
    setStandard("stdLRU");
}

// ── Suite drivers ───────────────────────────────────────────────────
//
// These build on registry.h (bench_registry()), export.h (exportJson(),
// exportMarkdown()), and helpers.h (setHeader(), setSuite()) — kept here
// rather than in helpers.h so that dependency only flows one way: this
// file already includes all three before any of the below is defined.

// Prints every registered suite's category header, one at a time, then
// exports the accumulated results as JSON and markdown.
inline void printAllBenchSuite() {
    for (const auto& suite : bench_registry()) {
        std::cout << "\n";
        setHeader(suite.name);
        setSuite(suite.name);
        suite.run();
    }
    std::cout << "\n";
    exportJson("benchmark_results.json");
    exportMarkdown("benchmark_results.md");
}

// Lists every registered suite, grouped by category, with its id and
// display name — does not run anything.
inline void printBenchSuiteList() {
    std::cout << "\nAvailable bench suites:\n";
    std::string category;

    for (const auto& suite : bench_registry()) {
        if (category != suite.category)
            std::cout << '\n' << CYAN << prettify(suite.category) << RESET << '\n';

        std::cout << GREEN << std::left << std::setw(6) << ("[" + suite.id + "]") << RESET
                  << std::setw(30) << suite.name << '\n';

        category = suite.category;
    }

    std::cout << "\n";
}

// Runs a single suite (matched by name, id, or category by the caller)
// and prints its table. Does not export — callers that use this run it
// per matching suite, then export once after the loop.
inline void printOneSuite(const BenchSuite& suite) {
    std::cout << "\n";
    setHeader(suite.name);
    setSuite(suite.name);
    suite.run();
    std::cout << "\n";
}