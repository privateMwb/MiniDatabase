#include "../../include/Core/Database.h"

#include <fstream>
#include <sstream>

// Constructors & Destructor
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
, dirty(other.dirty)
{
    other.dirty = false;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        for (Table* t : tables) {
            delete t;
        }
        
        name    = std::move(other.name);
        tables  = std::move(other.tables);
        index   = std::move(other.index);
        dirty   = other.dirty;
        
        other.dirty = false;
    }
    
    return *this;
}

// Table Management
Status Database::createTable(const std::string& name, VectorPro<ColumnDef> schema) {
    if (tables.size() >= DBConstants::MAX_TABLES)  return Status::OUT_OF_MEMORY;
    if (hasTable(name))                            return Status::TABLE_ALREADY_EXISTS;
    
    TableID tid = nextTableID();
    Table* table = new Table(name, tid, std::move(schema));
    
    tables.push_back(table);
    index.insert(name, tid);
    dirty = true;
    
    return Status::OK;
}

Status Database::dropTable(const std::string& name) {
    if(!hasTable(name)) return Status::TABLE_NOT_FOUND;
    
    TableID tid = index.at(name);
    
    tables.remove_if([tid](Table* t) {
        if (t->getID() == tid) {
            delete t;
            return true;
        }
        return false;
    });
    
    (void)index.erase(name);
    dirty = true;
    
    return Status::OK;
}

Table* Database::getTable(const std::string& name) {
    if(!hasTable(name)) return nullptr;
    
    TableID tid = index.at(name);
    for (Table* t : tables) {
        if(t->getID() == tid) return t;
    }
    
    return nullptr;
}

const Table* Database::getTable(const std::string& name) const {
    if(!hasTable(name)) return nullptr;
    
    TableID tid = index.at(name);
    for (Table* t : tables) {
        if(t->getID() == tid) return t;
    }
    
    return nullptr;
}

bool Database::hasTable(const std::string& name) const noexcept {
    return index.contains(name);
}

// Database Operations
Status Database::save(const std::string& path) const {
    Json envelope;
    envelope["__db_name__"] = name;;
    
    Json tableArr = Json(Json::ArrayType{});
    for (const Table* t : tables) {
        tableArr.asArray().push_back(Json::parse(t->serialize()));
    }
    envelope["tables"] = tableArr;
    
    std::ofstream file(path);
    if (!file.is_open()) return Status::IO_ERROR;
    
    file << envelope.dump();
    return Status::OK;
}

Status Database::load(const std::string& path) {
    std::ifstream file(path);
    if(!file.is_open()) return Status::IO_ERROR;
    
    std::stringstream ss;
    ss << file.rdbuf();
    
    Json envelope = Json::parse(ss.str());
    if(envelope.isNull()) return Status::PARSE_ERROR;
    
    name = envelope["__db_name__"].asString();
    
    for (Table* t : tables) delete t;
    tables = VectorPro<Table*>{};
    index.clear();
    
    for (const Json& tableJson : envelope["tables"].asArray()) {
        Table* t = new Table("", DBConstants::INVALID_TABLE_ID, VectorPro<ColumnDef>{});
        Status s = t->deserialize(tableJson.dump());
        if(s != Status::OK) {
            delete t;
            return s;
        }
        tables.push_back(t);
        index.insert(t->getName(), t->getID());
    }
    
    dirty = false;
    return Status::OK;
}

Status Database::compact() {
    for (Table* t : tables) {
        Status s = t->compact();
        if (s != Status::OK) return s;
    }
    
    dirty = false;
    return Status::OK;
}

// Introspection 
std::string Database::getName() const noexcept { return name; }
std::size_t Database::tableCount() const noexcept { return tables.size(); }
bool Database::isDirty() const noexcept { return dirty; }
bool Database::isEmpty() const noexcept { return tables.empty(); }
const VectorPro<Table*>& Database::getTables() const noexcept { return tables; }

std::size_t Database::recordCount() const noexcept {
    std::size_t count = 0;
    for(const Table* t : tables) {
        count += t->recordCount();
    }
    return count;
}

// Helper
TableID Database::nextTableID() const noexcept {
    return static_cast<TableID>(tables.size());
}









