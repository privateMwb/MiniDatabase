/**
 * @file Page.cpp
 * @brief MiniDB::Core::Page implementation.
 *
 * Contains the implementation of Page's record management, compaction,
 * and serialization.
 */

// ============================================================
// Implementation for MiniDB::Core::Page.
// ============================================================
//
//  Sections:
//   1. Constructors & Destructor
//   2. Record Operations
//   3. Page Management
//   4. Serialization
//   5. Introspection
//
// ============================================================

#include <MiniDB/Core/Page.h>

namespace MiniDB::Core {

// ============================================================
//  Section 1 — Constructors & Destructor
// ============================================================
Page::Page()
    : id(DBConstants::INVALID_PAGE_ID)
    , pool(sizeof(Record), DBConstants::MAX_RECORDS_PAGE) {}

Page::Page(PageID id)
    : id(id)
    , pool(sizeof(Record), DBConstants::MAX_RECORDS_PAGE) {}

Page::~Page() {
    for (Record* r : records) {
        pool.destroy(r);
    }
}

Page::Page(Page&& other) noexcept
    : id(other.id)
    , dirty(other.dirty)
    , records(std::move(other.records))
    , pool(std::move(other.pool))
{
    other.id = DBConstants::INVALID_PAGE_ID;
    other.dirty = false;
}

Page& Page::operator=(Page&& other) noexcept {
    if (this != &other) {
        for (Record* r : records) {
            pool.destroy(r);
        }

        id       = other.id;
        dirty    = other.dirty;
        records  = std::move(other.records);
        pool     = std::move(other.pool);

        other.id     = DBConstants::INVALID_PAGE_ID;
        other.dirty  = false;
    }

    return *this;
}


// ============================================================
//  Section 2 — Record Operations
// ============================================================
Status Page::addRecord(const Record& record) {
    if (isFull()) return Status::OUT_OF_MEMORY;

    Record* slot = pool.create<Record>(record.getID(), record.data);
    if (!slot) return Status::OUT_OF_MEMORY;

    records.push_back(slot);
    dirty = true;

    return Status::OK;
}

Record* Page::getRecord(RecordID id) {
    for (Record* r : records) {
        if (!r->isDeleted() && r->getID() == id) return r;
    }
    return nullptr;
}

Record* Page::getRecordAt(std::size_t index) {
    if (index >= records.size()) return nullptr;
    return records[index];
}

const Record* Page::getRecordAt(std::size_t index) const {
    if (index >= records.size()) return nullptr;
    return records[index];
}

const Record* Page::getRecord(RecordID id) const {
    for (Record* r : records) {
        if (!r->isDeleted() && r->getID() == id) return r;
    }
    return nullptr;
}

Status Page::updateRecord(const Record& record) {
    Record* existing = getRecord(record.getID());
    if (!existing) return Status::NOT_FOUND;

    existing->data  = record.data;
    dirty           = true;

    return Status::OK;
}

Status Page::deleteRecord(RecordID id) {
    Record* existing = getRecord(id);
    if (!existing) return Status::NOT_FOUND;

    // Soft delete: record stays in `records` (still occupies pool memory
    // and counts toward isFull()) until compact() reclaims it.
    existing->markDeleted();
    dirty = true;

    return Status::OK;
}


// ============================================================
//  Section 3 — Page Management
// ============================================================
Status Page::compact() {
    Vector<Record*> live;

    for (Record* r : records) {
        if (r->isDeleted()) {
            pool.destroy(r);
        } else {
            live.push_back(r);
        }
    }

    records = std::move(live);
    dirty = false;

    return Status::OK;
}


// ============================================================
//  Section 4 — Serialization
// ============================================================
Json Page::toJson() const {
    Json envelope(Json::ObjectType{});
    envelope["__page_id__"] = static_cast<int>(id);

    Json arr = Json(Json::ArrayType{});
    for (const Record* r : records) {
        if (!r->isDeleted())
            arr.asArray().push_back(r->toJson());   // was: Json::parse(r->serialize())
    }

    envelope["records"] = arr;
    return envelope;
}

Status Page::fromJson(const Json& envelope) {
    if (envelope.isNull()) return Status::PARSE_ERROR;

    id = static_cast<PageID>(envelope["__page_id__"].asNumber());
    const Json::ArrayType& arr = envelope["records"].asArray();

    for (const Json& entry : arr) {
        Record* slot = pool.create<Record>();
        if (!slot) return Status::OUT_OF_MEMORY;

        Status s = slot->fromJson(entry);   // was: slot->deserialize(entry.dump())
        if (s != Status::OK) {
            pool.destroy(slot);
            return s;
        }

        records.push_back(slot);
    }

    dirty = false;
    return Status::OK;
}

std::string Page::serialize() const {
    return toJson().dump();
}

Status Page::deserialize(const std::string& raw) {
    try {
        Json envelope = Json::parse(raw);

        if (envelope.isNull())
            return Status::PARSE_ERROR;

        return fromJson(envelope);
    }
    catch (...) {
        return Status::PARSE_ERROR;
    }
}


// ============================================================
//  Section 5 — Introspection
// ============================================================
bool         Page::isFull()       const noexcept { return records.size() == DBConstants::MAX_RECORDS_PAGE; }
bool         Page::isDirty()      const noexcept { return dirty; }
bool         Page::isEmpty()      const noexcept { return records.empty(); }
std::size_t  Page::recordCount()  const noexcept { return records.size(); }
std::size_t  Page::freeSlots()    const noexcept { return DBConstants::MAX_RECORDS_PAGE - records.size(); }
PageID       Page::getID()        const noexcept { return id; }

} // namespace MiniDB::Core
