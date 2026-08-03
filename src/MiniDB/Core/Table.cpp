/**
 * @file Table.cpp
 * @brief MiniDB::Core::Table implementation.
 *
 * Contains the implementation of Table's CRUD operations, page
 * management, indexing, and serialization.
 */

// ============================================================
// Implementation for MiniDB::Core::Table.
// ============================================================
//
//  Sections:
//   1. Constructors & Destructor
//   2. CRUD
//   3. Schema
//   4. Page Management
//   5. Index
//   6. Serialization
//   7. Introspection
//   8. Helper
//
// ============================================================

#include <MiniDB/Core/Table.h>

#include <algorithm>

namespace MiniDB::Core {

// ============================================================
//  Section 1 — Constructors & Destructor
// ============================================================
Table::Table(std::string name, TableID id, Vector<ColumnDef> schema)
    : name(std::move(name))
    , id(id)
    , schema(std::move(schema)) {}

Table::~Table() {
    for (Page* p : pages) {
        delete p;
    }
}

Table::Table(Table&& other) noexcept
    : name(std::move(other.name))
    , id(other.id)
    , schema(std::move(other.schema))
    , pages(std::move(other.pages))
    , index(std::move(other.index))
    , pageIndex(std::move(other.pageIndex))
    , nextPageId_(other.nextPageId_)
    , dirty(other.dirty)
{
    other.id          = DBConstants::INVALID_TABLE_ID;
    other.dirty       = false;
    other.nextPageId_ = 0;
}

Table& Table::operator=(Table&& other) noexcept {
    if (this != &other) {
        for (Page* p : pages) {
            delete p;
        }

        name        = std::move(other.name);
        id          = other.id;
        schema      = std::move(other.schema);
        pages       = std::move(other.pages);
        index       = std::move(other.index);
        pageIndex   = std::move(other.pageIndex);
        nextPageId_ = other.nextPageId_;
        dirty       = other.dirty;

        other.id          = DBConstants::INVALID_TABLE_ID;
        other.dirty       = false;
        other.nextPageId_ = 0;
    }

    return *this;
}


// ============================================================
//  Section 2 — CRUD
// ============================================================
Status Table::insertRecord(const Record& record) {
    Status s = record.validate(schema);
    if (s != Status::OK) return s;

    if (index.contains(record.getID())) return Status::DUPLICATE_KEY;

    Page* page = findPageWithSlot();
    if (!page) return Status::OUT_OF_MEMORY;

    s = page->addRecord(record);
    if (s != Status::OK) return s;

    index.insert(record.getID(), page->getID());
    dirty = true;

    return Status::OK;
}

Status Table::getRecord(RecordID id, Record& out) const {
    if (!index.contains(id)) return Status::NOT_FOUND;

    PageID pid = index.at(id);
    const Page* page = findPageByID(pid);
    if (!page) return Status::NOT_FOUND;

    const Record* r = page->getRecord(id);
    if (!r) return Status::NOT_FOUND;

    out = *r;
    return Status::OK;
}

Status Table::updateRecord(const Record& record) {
    Status s = record.validate(schema);
    if (s != Status::OK) return s;

    if (!index.contains(record.getID())) return Status::NOT_FOUND;

    PageID pid = index.at(record.getID());
    Page* page = findPageByID(pid);
    if (!page) return Status::NOT_FOUND;

    s = page->updateRecord(record);
    if (s != Status::OK) return s;

    dirty = true;
    return Status::OK;
}

Status Table::deleteRecord(RecordID id) {
    if (!index.contains(id)) return Status::NOT_FOUND;

    PageID pid = index.at(id);
    Page* page = findPageByID(pid);
    if (!page) return Status::NOT_FOUND;

    Status s = page->deleteRecord(id);
    if (s != Status::OK) return s;

    (void)index.erase(id);
    dirty = true;
    return Status::OK;
}


// ============================================================
//  Section 3 — Schema
// ============================================================
const Vector<ColumnDef>& Table::getSchema() const noexcept {
    return schema;
}

bool Table::hasColumn(const FieldName& name) const noexcept {
    for (const ColumnDef& col : schema) {
        if (col.name == name) return true;
    }

    return false;
}


// ============================================================
//  Section 4 — Page Management
// ============================================================
Status Table::compact() {
    for (Page* p : pages) {
        Status s = p->compact();
        if (s != Status::OK) return s;
    }

    dirty = false;
    return Status::OK;
}

const Vector<Page*>& Table::getPages() const noexcept {
    return pages;
}


// ============================================================
//  Section 5 — Index
// ============================================================
Status Table::rebuildIndex() {
    index.clear();
    pageIndex.clear();

    for (Page* p : pages) {
        pageIndex.insert(p->getID(), p);
        for (std::size_t i = 0; i < p->recordCount(); ++i) {
            const Record* r = p->getRecordAt(i);
            if (r && !r->isDeleted()) {
                index.insert(r->getID(), p->getID());
            }
        }
    }

    return Status::OK;
}


// ============================================================
//  Section 6 — Serialization
// ============================================================
Json Table::toJson() const {
    Json envelope(Json::ObjectType{});
    envelope["__table_id___"]  = static_cast<int>(id);
    envelope["__name__"]       = name;

    Json schemaArr = Json(Json::ArrayType{});
    for (const ColumnDef& col : schema) {
        Json colJson(Json::ObjectType{});
        colJson["name"]      = col.name;
        colJson["type"]      = static_cast<int>(col.type);
        colJson["nullable"]  = col.nullable;

        schemaArr.asArray().push_back(colJson);
    }
    envelope["schema"] = schemaArr;

    Json pageArr = Json(Json::ArrayType{});
    for (const Page* p : pages) {
        pageArr.asArray().push_back(p->toJson());   // was: Json::parse(p->serialize())
    }
    envelope["page"] = pageArr;

    return envelope;
}

Status Table::fromJson(const Json& envelope) {
    if (envelope.isNull()) return Status::PARSE_ERROR;

    id    = static_cast<TableID>(envelope["__table_id___"].asNumber());
    name  = envelope["__name__"].asString();

    schema = Vector<ColumnDef>{};
    for (const Json& col : envelope["schema"].asArray()) {
        ColumnDef def;
        def.name      = col["name"].asString();
        def.type      = static_cast<ColumnType>(static_cast<int>(col["type"].asNumber()));
        def.nullable  = col["nullable"].asBool();

        schema.push_back(def);
    }

    PageID maxPageId = 0;
    for (const Json& pageJson : envelope["page"].asArray()) {
        Page* p   = new Page(DBConstants::INVALID_PAGE_ID);
        Status s  = p->fromJson(pageJson);   // was: p->deserialize(pageJson.dump())
        if (s != Status::OK) {
            delete p;
            return s;
        }
        maxPageId = std::max(maxPageId, p->getID());
        pages.push_back(p);
    }
    // Resume the monotonic counter above every id we just loaded, so newly
    // inserted pages after a load can never collide with a loaded page id.
    nextPageId_ = pages.empty() ? 0 : static_cast<PageID>(maxPageId + 1);

    return rebuildIndex();
}

std::string Table::serialize() const {
    return toJson().dump();
}

Status Table::deserialize(const std::string& raw) {
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


// ============================================================
//  Section 7 — Introspection
// ============================================================
std::string  Table::getName()    const noexcept { return name; }
TableID      Table::getID()      const noexcept { return id; }
bool         Table::isDirty()    const noexcept { return dirty; }
bool         Table::isEmpty()    const noexcept { return pages.empty(); }
std::size_t  Table::pageCount()  const noexcept { return pages.size(); }

std::size_t Table::recordCount() const noexcept {
    std::size_t count = 0;
    for (const Page* p : pages) {
        count += p->recordCount();
    }

    return count;
}


// ============================================================
//  Section 8 — Helper
// ============================================================
Page* Table::findPageWithSlot() noexcept {
    for (Page* p : pages) {
        if (!p->isFull()) return p;
    }

    Page* newPage = new Page(nextPageID());
    pages.push_back(newPage);
    pageIndex.insert(newPage->getID(), newPage);
    return newPage;
}

Page* Table::findPageByID(PageID id) noexcept {
    // O(1) via pageIndex instead of a linear scan over `pages`.
    if (!pageIndex.contains(id)) return nullptr;
    return pageIndex.at(id);
}

const Page* Table::findPageByID(PageID id) const noexcept {
    if (!pageIndex.contains(id)) return nullptr;
    return pageIndex.at(id);
}

PageID Table::nextPageID() noexcept {
    // BUGFIX: was `pages.size()`, which is reused/collides once pages can be
    // removed (compact() never removes whole pages today, but this keeps the
    // id source correct independent of that). Monotonic, never reused.
    return nextPageId_++;
}

} // namespace MiniDB::Core
