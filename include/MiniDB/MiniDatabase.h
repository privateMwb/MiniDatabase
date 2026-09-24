/**
 * @file            MiniDatabase.h
 *
 * @date            2026-9-4
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 MWB
 *                  All rights reserved.
 *                  https://github.com/privateMwb/MiniDatabase
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

// clang-format off
#include <MiniDB/Common/Type.h>            // MiniDB::Common
#include <MiniDB/Common/FileIO.h>          // MiniDB::Common::FileIO
#include <MiniDB/Core/Record.h>            // MiniDB::Core
#include <MiniDB/Core/Page.h>
#include <MiniDB/Core/Table.h>
#include <MiniDB/Core/Database.h>
#include <MiniDB/Engine/StorageEngine.h>   // MiniDB::Engine
#include <MiniDB/Engine/QueryEngine.h>
#include <MiniDB/Engine/Serializer.h>
#include <MiniDB/Engine/Concurrency.h>
#include <MiniDB/Storage/WriteAheadLog.h>  // MiniDB::Storage
// clang-format on

// Single umbrella header pulling in every MiniDB sub-namespace and
// declaring the one rain reopen below. Users who only need one piece
// (e.g. just Database.h) can keep including that header directly and
// skip this one; this header exists for "give me everything" convenience
// and for the rain alias, which is deliberately declared in exactly one
// place rather than once per sub-namespace.

/**
 * @brief Umbrella alias so this library's types are reachable under
 *        `rain::`, alongside every other project library, while the
 *        true namespace (and all internal diagnostics) remains
 *        `MiniDB`.
 *
 * @details
 * `using namespace MiniDB;` only pulls in names declared directly in
 * `MiniDB` - since every actual type lives in a sub-namespace
 * (`MiniDB::Common`, `MiniDB::Common::FileIO`, `MiniDB::Core`,
 * `MiniDB::Engine`, `MiniDB::Storage`), this does NOT produce flat names
 * like `rain::Database` or `rain::Table`. What it gives you is the
 * sub-namespaces themselves, reachable one level under `rain` instead of
 * under `MiniDB`:
 * `rain::Common::ColumnType`, `rain::Common::FileIO::readFile`,
 * `rain::Core::Database`, `rain::Core::Table`, `rain::Core::Page`,
 * `rain::Core::Record`, `rain::Engine::StorageEngine`,
 * `rain::Engine::QueryEngine`, `rain::Engine::Serializer`,
 * `rain::Engine::Concurrency`, `rain::Storage::WriteAheadLog`.
 *
 * Declared here only, in this umbrella header - no other MiniDB header
 * reopens `rain`.
 */
namespace rain {
using namespace MiniDB;
}
