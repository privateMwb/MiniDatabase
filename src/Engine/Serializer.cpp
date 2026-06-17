#include "../../include/Engine/Serializer.h"

#include <fstream>
#include <sstream>

// Table Export/Import
std::string Serializer::exportTableToJson(const Table& table) {
    Json arr = Json(Json::ArrayType{});
    
    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecord(i);
            if (!r || r->isDeleted()) continue;
            
            Json entry = r->data;
            entry["id"] = static_cast<int>(r->getID());
            arr.asArray().push_back(entry);
        }
    }
    return arr.dump();
}

Status Serializer::importTableFromJson(Table& table, const std::string& json) {
    Json parsed = Json::parse(json);
    if(parsed.isNull())    return Status::PARSE_ERROR;
    if(!parsed.isArray())  return Status::PARSE_ERROR;
    
    RecordID nextId = 0;
    for (const Json& entry : parsed.asArray()) {
        RecordID id = nextId++;
        
        Json cleanData = entry;
        
        if (entry.contains("id") && entry["id"].isNumber()) {
            id = static_cast<RecordID>(entry["id"].asNumber());
            cleanData.asObject().erase("id");
        }
        
        Record r(id, cleanData);
        Status s = table.insertRecord(r);
        if (s != Status::OK) return s;
    }
    return Status::OK;
}

// File-based Export/Import 
Status Serializer::exportTableToFile(const Table& table, const std::string& path) {
    std::string json = exportTableToJson(table);
    
    std::ofstream file(path);
    if (!file.is_open()) return Status::IO_ERROR;
    
    file << json;
    return Status::OK;
}

Status Serializer::importTableFromFile(Table& table, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return Status::IO_ERROR;
    
    std::stringstream ss;
    ss << file.rdbuf();
    
    return importTableFromJson(table, ss.str());
}

// Database-wide Export/Import 
Status Serializer::exportDatabaseToJson(const Database& db, const std::string& path) {
    Json envelope;
    
    for (const Table* t : db.getTables()) {
        Json tableArr = Json(Json::ArrayType{});
        for (const Page* page : t->getPages()) {
            for (std::size_t i = 0; i < page->recordCount(); ++i) {
                const Record* r = page->getRecord(i);
                if (!r || r->isDeleted()) continue;
                
                Json entry = r->data;
                entry["id"] = static_cast<int>(r->getID());
                tableArr.asArray().push_back(entry);
            }
        }
        envelope[t->getName()] = tableArr;
    }
    
    std::ofstream file(path);
    if(!file.is_open()) return Status::IO_ERROR;
    
    file << envelope.dump();
    return Status::OK;
}

Status Serializer::importDatabaseFromJson(Database& db, const std::string& path) {
    std::ifstream file(path);
    if(!file.is_open()) return Status::IO_ERROR;
    
    std::stringstream ss;
    ss << file.rdbuf();
    
    Json envelope = Json::parse(ss.str());
    if (envelope.isNull()) return Status::PARSE_ERROR;
    
    for (const auto& [tableName, tableData] : envelope.asObject()) {
        if (!db.hasTable(tableName)) continue;
        
        Table* table = db.getTable(tableName);
        if (!table) continue;
        
        Status s = importTableFromJson(*table, tableData.dump());
        if (s != Status::OK) return s;
    }
    
    return Status::OK;
}











