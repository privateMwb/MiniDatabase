#pragma once

#include <string>
#include <future>

#include "../Common/Type.h"
#include "../Core/Database.h"
#include "../Core/Table.h"
#include "Serializer.h"

#include "../../libs/PulseThreadPool/ThreadPool.h"

class Concurrency { 
    private:
    
    // Pool 
    ThreadPool pool;
    
    public:
    
    // Constructor
    explicit Concurrency(std::size_t threadCount = DBConstants::THREAD_POOL_SIZE);
    
    // Parallel Save/Load
    [[nodiscard]] Status saveAllTablesParallel(
        const Database&     db,
        const std::string&  baseFilename
    );
    
    [[nodiscard]] Status loadAllTablesParallel(
        Database&           db,
        const std::string&  baseFilename
    );
    
    // Parallel Index Rebuilding
    [[nodiscard]] Status rebuildAllIndexesParallel(Database& db);
    
    // Parallel Export
    [[nodiscard]] Status exportAllTablesParallel(
        const Database&     db,
        const std::string&  outputDirectory
    );
    
    // Introspection 
    [[nodiscard]] std::size_t activeTasks()   const noexcept;
    [[nodiscard]] std::size_t queuedTasks()   const noexcept;
    [[nodiscard]] std::size_t threadCount()  const noexcept;
};
