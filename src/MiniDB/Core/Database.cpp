/**
 * @file Database.cpp
 * @brief MiniDB::Core::Database implementation.
 *
 * Contains the implementation of Database's table management,
 * whole-database persistence, and serialization.
 */

// ============================================================
// Implementation for MiniDB::Core::Database.
// ============================================================
//
//  Sections:
//   1. Constructors & Destructor
//   2. Table Management
//   3. Serialization
//   4. Database Operations
//   5. Introspection
//   6. Helper
//
// ============================================================

#include <MiniDB/Core/Database.h>
#include <MiniDB/Common/FileIO.h>

#include <algorithm>

namespace MiniDB::Core {

// ============================================================
//  Section 1 — Constructors & Destructor
// ============================================================
Database::Database(std::string name)
    : name(std::move(name)) {}

Database::~Database() {
    for (Table* t : tables) {
        delete t;
    }
}

Database::Database(Database&& other) noexcept
    : name(std::move(other.name))
    , tables(std::move(other.tables))
    , index(std::move(other.index))
    , tableIndex(std::move(other.tableIndex))
    , nextTableId_(other.nextTableId_)
    , dirty(other.dirty)
{
    other.tables.clear();
    other.index.clear();
    other.tableIndex.clear();
    other.dirty        = false;
    other.nextTableId_ = 0;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        for (Table* t : tables) {
            delete t;
        }

        name         = std::move(other.name);
        tables       = std::move(other.tables);
        index        = std::move(other.index);
        tableIndex   = std::move(other.tableIndex);
        nextTableId_ = other.nextTableId_;
        dirty        = other.dirty;

        other.tables.clear();
        other.index.clear();
        other.tableIndex.clear();
        other.dirty        = false;
        other.nextTableId_ = 0;
    }

    return *this;
}


// ============================================================
//  Section 2 — Table Management
// ============================================================
Status Database::createTable(const std::string& name, Vector<ColumnDef> schema) {
    if (tables.size() >= DBConstants::MAX_TABLES)  return Status::OUT_OF_MEMORY;
    if (hasTable(name))                            return Status::TABLE_ALREADY_EXISTS;

    TableID tid = nextTableID();
    Table* table = new Table(name, tid, std::move(schema));

    tables.push_back(table);
    index.insert(name, tid);
    tableIndex.insert(tid, table);
    dirty = true;

    return Status::OK;
}

Status Database::dropTable(const std::string& name) {
    if (!hasTable(name)) return Status::TABLE_NOT_FOUND;

    TableID tid = index.at(name);

    (void)tables.remove_if([tid](Table* t) {
        if (t->getID() == tid) {
            delete t;
            return true;
        }
        return false;
    });

    (void)index.erase(name);
    (void)tableIndex.erase(tid);
    dirty = true;

    return Status::OK;
}

Table* Database::getTable(const std::string& name) {
    if (!hasTable(name)) return nullptr;
    // O(1) via tableIndex instead of a linear scan over `tables`.
    TableID tid = index.at(name);
    if (!tableIndex.contains(tid)) return nullptr;
    return tableIndex.at(tid);
}

const Table* Database::getTable(const std::string& name) const {
    if (!hasTable(name)) return nullptr;
    TableID tid = index.at(name);
    if (!tableIndex.contains(tid)) return nullptr;
    return tableIndex.at(tid);
}

bool Database::hasTable(const std::string& name) const noexcept {
    return index.contains(name);
}


// ============================================================
//  Section 3 — Serialization
// ============================================================
Json Database::toJson() const {
    Json envelope(Json::ObjectType{});
    envelope["__db_name__"] = name;

    Json tableArr = Json(Json::ArrayType{});
    for (const Table* t : tables) {
        tableArr.asArray().push_back(t->toJson());   // was: Json::parse(t->serialize())
    }
    envelope["tables"] = tableArr;

    return envelope;
}

Status Database::fromJson(const Json& envelope) {
    if (envelope.isNull()) return Status::PARSE_ERROR;

    if (envelope["tables"].asArray().size() > DBConstants::MAX_TABLES) {
        return Status::OUT_OF_MEMORY;
    }

    // Build into scratch containers first; only swap into `*this` once every
    // table has parsed successfully. Prevents a mid-load failure from
    // leaving the database in a partial, inconsistent state (previously
    // `tables` was cleared up front and populated in place, so a failure on
    // table K left tables 0..K-1 loaded and the rest missing).
    Vector<Table*>                scratchTables;
    HashMap<std::string, TableID> scratchIndex;
    HashMap<TableID, Table*>      scratchTableIndex;
    TableID                       maxTableId = 0;

    auto cleanup = [&scratchTables] {
        for (Table* t : scratchTables) delete t;
    };

    for (const Json& tableJson : envelope["tables"].asArray()) {
        Table* t = new Table("", DBConstants::INVALID_TABLE_ID, Vector<ColumnDef>{});
        Status s = t->fromJson(tableJson);   // was: t->deserialize(tableJson.dump())
        if (s != Status::OK) {
            delete t;
            cleanup();
            return s;
        }
        maxTableId = std::max(maxTableId, t->getID());
        scratchIndex.insert(t->getName(), t->getID());
        scratchTableIndex.insert(t->getID(), t);
        scratchTables.push_back(t);
    }

    // Success: replace live state.
    for (Table* t : tables) delete t;

    name         = envelope["__db_name__"].asString();
    tables       = std::move(scratchTables);
    index        = std::move(scratchIndex);
    tableIndex   = std::move(scratchTableIndex);
    nextTableId_ = tables.empty() ? 0 : static_cast<TableID>(maxTableId + 1);
    dirty        = false;

    return Status::OK;
}


// ============================================================
//  Section 4 — Database Operations
// ============================================================
Status Database::save(const std::string& path) const {
    // Single dump() at the outermost boundary (toJson() builds the tree with
    // no intermediate per-table/per-page/per-record dump+parse round trips),
    // written atomically (temp file + fsync + rename) so a crash or I/O
    // failure mid-write can never corrupt the existing on-disk database.
    return FileIO::writeFileAtomic(path, toJson().dump());
}

Status Database::load(const std::string& path) {
    std::string raw;
    if (Status s = FileIO::readFile(path, raw); s != Status::OK)
        return s;

    try {
        Json envelope = Json::parse(raw);

        if (envelope.isNull())
            return Status::PARSE_ERROR;

        return fromJson(envelope);
    }
    catch (const std::exception&) {
        return Status::PARSE_ERROR;
    }
}

Status Database::compact() {
    for (Table* t : tables) {
        Status s = t->compact();
        if (s != Status::OK) return s;
    }

    dirty = false;
    return Status::OK;
}


// ============================================================
//  Section 5 — Introspection
// ============================================================
std::string Database::getName() const noexcept { return name; }
std::size_t Database::tableCount() const noexcept { return tables.size(); }
bool Database::isDirty() const noexcept { return dirty; }
bool Database::isEmpty() const noexcept { return tables.empty(); }
const Vector<Table*>& Database::getTables() const noexcept { return tables; }

std::size_t Database::recordCount() const noexcept {
    std::size_t count = 0;
    for (const Table* t : tables) {
        count += t->recordCount();
    }
    return count;
}


// ============================================================
//  Section 6 — Helper
// ============================================================
TableID Database::nextTableID() noexcept {
    // BUGFIX: was `tables.size()`, which is reused/collides after
    // dropTable() shrinks the vector (create-after-drop could reuse an id
    // still held by a surviving table, causing getTable() to resolve to the
    // wrong table). Monotonic, never reused.
    return nextTableId_++;
}

} // namespace MiniDB::Core
