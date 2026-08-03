/**
 * @file Serializer.cpp
 * @brief MiniDB::Engine::Serializer implementation.
 *
 * Contains the implementation of the flat array-of-records export/import
 * format, for both individual tables and whole databases.
 */

// ============================================================
// Implementation for MiniDB::Engine::Serializer.
// ============================================================
//
//  Sections:
//   1. Table Export/Import
//   2. File-based Export/Import
//   3. Database-wide Export/Import
//
// ============================================================

#include <MiniDB/Engine/Serializer.h>
#include <MiniDB/Common/FileIO.h>

namespace MiniDB::Engine {

// ============================================================
//  Section 1 — Table Export/Import
// ============================================================
std::string Serializer::exportTableToJson(const Table& table) {
    Json arr = Json(Json::ArrayType{});

    for (const Page* page : table.getPages()) {
        for (std::size_t i = 0; i < page->recordCount(); ++i) {
            const Record* r = page->getRecordAt(i);
            if (!r || r->isDeleted()) continue;

            Json entry = r->data;
            entry["id"] = static_cast<int>(r->getID());
            arr.asArray().push_back(entry);
        }
    }
    return arr.dump();
}

Status Serializer::importTableFromJson(Table& table, const Json& parsed) {
    if (parsed.isNull())    return Status::PARSE_ERROR;
    if (!parsed.isArray())  return Status::PARSE_ERROR;

    RecordID nextId = 0;
    for (const Json& entry : parsed.asArray()) {
        RecordID id = nextId++;

        Json cleanData = entry;

        // "id" is metadata added on export, not part of the record's own
        // fields — strip it back out before storing so re-importing an
        // exported table doesn't leave a stray "id" field in the data.
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

Status Serializer::importTableFromJson(Table& table, const std::string& json) {
    Json parsed = Json::parse(json);
    return importTableFromJson(table, parsed);
}


// ============================================================
//  Section 2 — File-based Export/Import
// ============================================================
Status Serializer::exportTableToFile(const Table& table, const std::string& path) {
    return FileIO::writeFileAtomic(path, exportTableToJson(table));
}

Status Serializer::importTableFromFile(Table& table, const std::string& path) {
    std::string raw;
    // Sized single read instead of stringstream/rdbuf (avoids per-char
    // streambuf overhead on large files).
    if (Status s = FileIO::readFile(path, raw); s != Status::OK) return s;
    return importTableFromJson(table, raw);
}


// ============================================================
//  Section 3 — Database-wide Export/Import
// ============================================================
Status Serializer::exportDatabaseToJson(const Database& db, const std::string& path) {
    Json envelope(Json::ObjectType{});

    for (const Table* t : db.getTables()) {
        Json tableArr = Json(Json::ArrayType{});
        for (const Page* page : t->getPages()) {
            for (std::size_t i = 0; i < page->recordCount(); ++i) {
                const Record* r = page->getRecordAt(i);
                if (!r || r->isDeleted()) continue;

                Json entry = r->data;
                entry["id"] = static_cast<int>(r->getID());
                tableArr.asArray().push_back(entry);
            }
        }
        envelope[t->getName()] = tableArr;
    }

    return FileIO::writeFileAtomic(path, envelope.dump());
}

Status Serializer::importDatabaseFromJson(Database& db, const std::string& path) {
    std::string raw;
    if (Status s = FileIO::readFile(path, raw); s != Status::OK)
        return s;

    Json envelope = Json::parse(raw);
    if (envelope.isNull())
        return Status::PARSE_ERROR;

    const auto& object = envelope.asObject();

    for (const auto& [tableName, tableData] : object.entries()) {
        if (!db.hasTable(tableName))
            continue;

        Table* table = db.getTable(tableName);
        if (!table)
            continue;

        // Avoid reparsing by importing directly from the parsed subtree.
        Status s = importTableFromJson(*table, tableData);
        if (s != Status::OK)
            return s;
    }

    return Status::OK;
}
} // namespace MiniDB::Engine
