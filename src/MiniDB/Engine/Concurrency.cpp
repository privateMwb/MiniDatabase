/**
 * @file Concurrency.cpp
 * @brief MiniDB::Engine::Concurrency implementation.
 *
 * Contains the implementation of parallel table save/load, index
 * rebuilding, and export, fanned out across a ThreadPoolPro::ThreadPool.
 */

// ============================================================
// Implementation for MiniDB::Engine::Concurrency.
// ============================================================
//
//  Sections:
//   1. Helpers
//   2. Constructor
//   3. Parallel Save/Load
//   4. Parallel Index Rebuilding
//   5. Parallel Export
//   6. Introspection
//
// ============================================================

#include <MiniDB/Engine/Concurrency.h>

#include <vector>

namespace MiniDB::Engine {

// ============================================================
//  Section 1 — Helpers
// ============================================================
namespace {
// Shared enqueue/collect pattern used by every parallel operation below
// (previously duplicated 4x). `fn` is applied to each table on the pool;
// results are collected and folded into a single Status (last non-OK wins,
// matching the original per-call behavior).
//
// BUGFIX: previously stored results in std::vector<std::future<Status>>,
// assuming ThreadPool::enqueue() returns a standard std::future. It
// actually returns ThreadPoolPro's own Detail::Future<T> (templated only
// on the callable's return type, so it's the same Future<Status> for
// every table here regardless of each lambda's distinct closure type).
// FutureType is deduced via decltype instead of naming that type directly,
// so this doesn't hardcode a ThreadPoolPro-internal type name.
template <class F>
Status runParallel(ThreadPool& pool, const Vector<Table*>& tables, F&& fn) {
    using FutureType = decltype(pool.enqueue([]() -> Status { return Status::OK; }));

    std::vector<FutureType> futures;
    futures.reserve(tables.size());

    for (Table* t : tables) {
        futures.push_back(pool.enqueue([t, fn]() -> Status { return fn(t); }));
    }

    Status finalStatus = Status::OK;
    for (auto& f : futures) {
        if (Status s = f.get(); s != Status::OK) finalStatus = s;
    }
    return finalStatus;
}
} // namespace


// ============================================================
//  Section 2 — Constructor
// ============================================================
Concurrency::Concurrency(std::size_t threadCount)
    : pool(threadCount) {}


// ============================================================
//  Section 3 — Parallel Save/Load
// ============================================================
Status Concurrency::saveAllTablesParallel(
    const Database&     db,
    const std::string&  baseFilename)
{
    return runParallel(pool, db.getTables(), [&baseFilename](Table* t) -> Status {
        std::string filename = baseFilename + "_" + t->getName() + ".json";
        return Serializer::exportTableToFile(*t, filename);
    });
}

Status Concurrency::loadAllTablesParallel(
    Database&           db,
    const std::string&  baseFilename)
{
    return runParallel(pool, db.getTables(), [&baseFilename](Table* t) -> Status {
        std::string filename = baseFilename + "_" + t->getName() + ".json";
        return Serializer::importTableFromFile(*t, filename);
    });
}


// ============================================================
//  Section 4 — Parallel Index Rebuilding
// ============================================================
Status Concurrency::rebuildAllIndexesParallel(Database& db) {
    return runParallel(pool, db.getTables(), [](Table* t) -> Status {
        return t->rebuildIndex();
    });
}


// ============================================================
//  Section 5 — Parallel Export
// ============================================================
Status Concurrency::exportAllTablesParallel(
    const Database&     db,
    const std::string&  outputDirectory)
{
    return runParallel(pool, db.getTables(), [&outputDirectory](Table* t) -> Status {
        std::string path = outputDirectory + "/" + t->getName() + ".json";
        return Serializer::exportTableToFile(*t, path);
    });
}


// ============================================================
//  Section 6 — Introspection
// ============================================================
std::size_t Concurrency::activeTasks() const noexcept { return pool.activeTaskCount(); }
std::size_t Concurrency::queuedTasks() const noexcept { return pool.queuedTasks(); }
std::size_t Concurrency::threadCount() const noexcept { return pool.threadCount(); }

} // namespace MiniDB::Engine
