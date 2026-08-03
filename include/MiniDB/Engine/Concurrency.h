/**
 * @file            Concurrency.h
 *
 * @date            2026-2-8
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 MWB
 *                  All rights reserved.
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

// clang-format off
#include <string> // std::string
#include <future> // std::future-adjacent return types used by ThreadPoolPro
// clang-format on

#include <MiniDB/Common/Type.h>
#include <MiniDB/Core/Database.h>
#include <MiniDB/Core/Table.h>
#include <MiniDB/Engine/Serializer.h>

#include <ThreadPoolPro/ThreadPool.h>

// Fans out per-table work (save/load/export/reindex) across a
// ThreadPoolPro::ThreadPool, one task per table. Every operation here
// follows the same shape: enqueue one closure per table, wait on every
// resulting future, and fold the individual Status results into one
// (last non-OK wins) -- see runParallel() in Concurrency.cpp.

namespace MiniDB::Engine {

using namespace MiniDB::Common;
using namespace MiniDB::Core;
using namespace ThreadPoolPro;

/**
 * @brief Runs per-table `Database` operations (save, load, export,
 * reindex) concurrently across a fixed-size thread pool.
 * @details Every method here enqueues one task per table onto the same
 * `ThreadPool` and blocks until all of them complete, so parallelism is
 * bounded by table count and pool size but each call itself is
 * synchronous from the caller's point of view. If any table's task
 * fails, the last non-`Status::OK` result is returned — all tasks still
 * run to completion, none are cancelled early.
 */
class Concurrency {
  private:
    ThreadPool pool; ///< Worker pool shared by every parallel operation below.

  public:
    /**
     * @brief Constructs a `Concurrency` backed by a thread pool of the
     * given size.
     * @param threadCount Number of worker threads. Defaults to
     * `DBConstants::THREAD_POOL_SIZE`.
     */
    explicit Concurrency(std::size_t threadCount = DBConstants::THREAD_POOL_SIZE);

    /**
     * @brief Saves every table in `db` to its own file, in parallel.
     * @param db Database whose tables should be saved.
     * @param baseFilename Filename prefix; each table is written to
     * `baseFilename + "_" + table.getName() + ".json"`.
     * @return `Status::OK` if every table saved successfully, otherwise
     * the last non-OK `Status` encountered.
     */
    [[nodiscard]] Status saveAllTablesParallel(const Database& db, const std::string& baseFilename);

    /**
     * @brief Loads every table in `db` from its own file, in parallel.
     * @param db Database whose tables should be loaded into.
     * @param baseFilename Filename prefix; each table is read from
     * `baseFilename + "_" + table.getName() + ".json"`.
     * @return `Status::OK` if every table loaded successfully, otherwise
     * the last non-OK `Status` encountered.
     */
    [[nodiscard]] Status loadAllTablesParallel(Database& db, const std::string& baseFilename);

    /**
     * @brief Rebuilds every table's record/page index in `db`, in
     * parallel.
     * @param db Database whose tables should be reindexed.
     * @return `Status::OK`, always — see `Table::rebuildIndex()`.
     */
    [[nodiscard]] Status rebuildAllIndexesParallel(Database& db);

    /**
     * @brief Exports every table in `db` to `outputDirectory`, in
     * parallel.
     * @param db Database whose tables should be exported.
     * @param outputDirectory Destination directory; each table is
     * written to `outputDirectory + "/" + table.getName() + ".json"`.
     * @return `Status::OK` if every table exported successfully,
     * otherwise the last non-OK `Status` encountered.
     */
    [[nodiscard]] Status exportAllTablesParallel(const Database& db,
                                                 const std::string& outputDirectory);

    /// @brief Returns the number of tasks currently executing on the pool.
    [[nodiscard]] std::size_t activeTasks() const noexcept;
    /// @brief Returns the number of tasks currently waiting to run.
    [[nodiscard]] std::size_t queuedTasks() const noexcept;
    /// @brief Returns the number of worker threads in the pool.
    [[nodiscard]] std::size_t threadCount() const noexcept;
};

} // namespace MiniDB::Engine
