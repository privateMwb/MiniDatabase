#include "../../include/Engine/Concurrency.h"

// Constructor
Concurrency::Concurrency(std::size_t threadCount)
	: pool(threadCount) {}

// Parallel Save/Load
Status Concurrency::saveAllTablesParallel(
    const Database&     db,
    const std::string&  baseFilename) 
{
    VectorPro<std::future<Status>> futures;
    
    for (const Table* t : db.getTables()) {
        std::string filename = baseFilename + "_" + t->getName() + ".json";
        
        futures.push_back(
            pool.enqueue([t, filename]() -> Status {
                return Serializer::exportTableToFile(*t, filename);
            })
        );
    }
    
    Status finalStatus = Status::OK;
    for (auto& f : futures) {
        Status s = f.get();
        if (s != Status::OK) finalStatus = s;
    }
    
    return finalStatus;
}

Status Concurrency::loadAllTablesParallel(
    Database&           db,
    const std::string&  baseFilename)
{
    VectorPro<std::future<Status>> futures;
    
    for (Table* t : db.getTables()) {
        std::string filename = baseFilename + "_" + t->getName() + ".json";
        
        futures.push_back(
            pool.enqueue([t, filename]() -> Status {
                return Serializer::importTableFromFile(*t, filename);
            })
        );
    }
    
    Status finalStatus = Status::OK;
    for (auto& f : futures) {
        Status s = f.get();
        if (s != Status::OK) finalStatus = s;
    }
    
    return finalStatus;
}

// Parallel Index Rebuilding
Status Concurrency::rebuildAllIndexesParallel(Database& db) {
    VectorPro<std::future<Status>> futures;
    
    for (Table* t : db.getTables()) {
        futures.push_back(
            pool.enqueue([t]() -> Status {
                return t->rebuildIndex();
            })
        );
    }
    
    Status finalStatus = Status::OK;
    for (auto& f : futures) {
        Status s = f.get();
        if (s != Status::OK) finalStatus = s;
    }
    
    return finalStatus;
}

// Parallel Export
Status Concurrency::exportAllTablesParallel(
    const Database&     db,
    const std::string&  outputDirectory)
{
    VectorPro<std::future<Status>> futures;
    
    for (const Table* t : db.getTables()) {
        std::string path = outputDirectory + "/" + t->getName() + ".json";
        
        futures.push_back(
            pool.enqueue([t, path]() -> Status {
                return Serializer::exportTableToFile(*t, path);
            })
        );
    }
    
    Status finalStatus = Status::OK;
    for (auto& f : futures) {
        Status s = f.get();
        if (s != Status::OK) finalStatus = s;
    }
    
    return finalStatus;
}

// Introspection
std::size_t Concurrency::activeTasks() const noexcept { return pool.activeTaskCount(); }
std::size_t Concurrency::queuedTasks() const noexcept { return pool.queuedTasks(); }
std::size_t Concurrency::threadCount() const noexcept { return pool.threadCount(); }







