#pragma once 

#include <string>

#include "../Common/Type.h"
#include "../Core/Database.h"
#include "../Core/Table.h"
#include "../Core/Record.h"

#include "../../libs/JsonParser/Json.h"

class Serializer {
    public:
    
    // Table Export/Import
    [[nodiscard]] static std::string exportTableToJson(const Table& table);
    [[nodiscard]] static Status      importTableFromJson(Table& table, const std::string& json);
    
    // File-based Export/Import 
    [[nodiscard]] static Status exportTableToFile(const Table& table, const std::string& path);
    [[nodiscard]] static Status importTableFromFile(Table& table, const std::string& path);
    
    // Database-wired Export/Import 
    [[nodiscard]] static Status exportDatabaseToJson(const Database& db, const std::string& path);
    [[nodiscard]] static Status importDatabaseFromJson(Database& db, const std::string& path);
};
