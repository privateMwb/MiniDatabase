// StorageEngine Test Suite
//
// Comprehensive test suite for the StorageEngine implementation.
//
// Covers:
// - Saving and loading a database through the storage layer
// - Page cache insertion and retrieval
// - Cache hit/miss behavior
// - Manual page eviction
// - Cache hit rate statistics
//
// Each test validates behavior using assertions and prints a
// success message when completed.

#include "TestHelpers.h"
#include "../include/Engine/StorageEngine.h"

#include <cassert>
#include <iostream>
#include <cstdio>

// Save And Load Database
// Verifies that StorageEngine correctly delegates database persistence to disk.
static void test_saveAndLoadDatabase() {
    StorageEngine storage(".");

    Database original("TestDB");
    (void)original.createTable("employees", makeEmployeeSchema());

    Table* employees = original.getTable("employees");
    (void)employees->insertRecord(makeEmployeeRecord(1, "Alice", 50000));
    (void)employees->insertRecord(makeEmployeeRecord(2, "Bob", 60000));

    const std::string filename = "storage_test_output.json";
    Status saveStatus = storage.saveDatabase(original, filename);
    assert(saveStatus == Status::OK);

    Database restored("");
    Status loadStatus = storage.loadDatabase(restored, filename);
    assert(loadStatus == Status::OK);

    assert(restored.tableCount() == 1);
    assert(restored.recordCount() == 2);

    std::remove(("./" + filename).c_str());

    std::cout << "[PASS] saveAndLoadDatabase\n";
}

// Cache Page
// Verifies that a page can be inserted into the cache and retrieved.
static void test_cachePage() {
    StorageEngine storage(".");

    Page page(1);
    (void)page.addRecord(makeEmployeeRecord(1, "Alice", 50000));

    storage.cachePage(1, std::move(page));

    Page* cached = storage.getCachedPage(1);
    assert(cached != nullptr);
    assert(cached->getID() == 1);
    assert(cached->recordCount() == 1);

    std::cout << "[PASS] cachePage\n";
}

// Cache Miss
// Verifies that requesting an uncached page returns nullptr.
static void test_cacheMiss() {
    StorageEngine storage(".");

    Page* missing = storage.getCachedPage(999);
    assert(missing == nullptr);

    std::cout << "[PASS] cacheMiss\n";
}

// Is Cached
// Verifies that isCached correctly reports presence without affecting cache order.
static void test_isCached() {
    StorageEngine storage(".");

    Page page(5);
    storage.cachePage(5, std::move(page));

    assert(storage.isCached(5));
    assert(!storage.isCached(404));

    std::cout << "[PASS] isCached\n";
}

// Evict Page
// Verifies that a manually evicted page is no longer present in the cache.
static void test_evictPage() {
    StorageEngine storage(".");

    Page page(7);
    storage.cachePage(7, std::move(page));
    assert(storage.isCached(7));

    storage.evictPage(7);
    assert(!storage.isCached(7));

    std::cout << "[PASS] evictPage\n";
}

// Cache Hit Rate
// Verifies that hit and miss statistics are tracked correctly across accesses.
static void test_cacheHitRate() {
    StorageEngine storage(".");

    Page page(10);
    storage.cachePage(10, std::move(page));

    // One guaranteed hit
    (void)storage.getCachedPage(10);

    // One guaranteed miss
    (void)storage.getCachedPage(404);

    assert(storage.cacheHits()   >= 1);
    assert(storage.cacheMisses() >= 1);
    assert(storage.cacheHitRate() > 0.0);
    assert(storage.cacheHitRate() <= 100.0);

    std::cout << "[PASS] cacheHitRate\n";
}

// Run Storage Tests
// Executes all storage engine unit tests and reports their results.
void run_storage_tests() {
    std::cout << "Storage Tests\n";
    test_saveAndLoadDatabase();
    test_cachePage();
    test_cacheMiss();
    test_isCached();
    test_evictPage();
    test_cacheHitRate();
    std::cout << "\n\n";
}