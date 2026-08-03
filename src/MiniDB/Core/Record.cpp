/**
 * @file Record.cpp
 * @brief MiniDB::Core::Record implementation.
 *
 * Contains the implementation of Record's field access, schema
 * validation, serialization, and comparison operators.
 */

// ============================================================
// Implementation for MiniDB::Core::Record.
// ============================================================
//
//  Sections:
//   1. Constructors
//   2. Field Access
//   3. Schema Validation
//   4. Serialization
//   5. Utility
//   6. Operators
//
// ============================================================

#include <MiniDB/Core/Record.h>

// clang-format off
#include <cmath>  // std::trunc
#include <limits>  // std::numeric_limits
// clang-format on

namespace MiniDB::Core {

// ============================================================
//  Section 1 — Constructors
// ============================================================
Record::Record()
    : id(DBConstants::INVALID_RECORD_ID)
    , data(Json::ObjectType{}) {}

Record::Record(RecordID id)
    : id(id)
    , data(Json::ObjectType{}) {}

Record::Record(RecordID id, Json data)
    : id(id)
    , data(std::move(data)) {}


// ============================================================
//  Section 2 — Field Access
// ============================================================
Status Record::setField(const FieldName& key, const Json& value) {
    if (key.empty()) return Status::INVALID_SCHEMA;
    data[key] = value;
    return Status::OK;
}

Json Record::getField(const FieldName& key) const {
    if (!data.contains(key)) return Json{};
    return data[key];
}

const Json& Record::getFieldRef(const FieldName& key) const {
    // NOTE: this only eliminates the copy at Record's call sites. Whether it
    // eliminates the copy end-to-end depends on JsonPro::Json::operator[]
    // const returning Json& (zero-copy) vs Json (still a copy internally,
    // just bound to a temporary here) -- confirm against JsonPro's header.
    static const Json kNull{};
    if (!data.contains(key)) return kNull;
    return data[key];
}

bool Record::hasField(const FieldName& key) const {
    return data.contains(key);
}

Status Record::removeField(const FieldName& key) {
    if (!data.contains(key)) return Status::NOT_FOUND;
    data.asObject().erase(key);
    return Status::OK;
}


// ============================================================
//  Section 3 — Schema Validation
// ============================================================
Status Record::validate(const Vector<ColumnDef>& schema) const {
    for (const auto& col : schema) {
        if (!data.contains(col.name)) {
            if (!col.nullable) return Status::INVALID_SCHEMA;
            continue;
        }

        const Json& val = data[col.name];
        bool typeOK = false;

        switch (col.type) {
            case ColumnType::INT: {
                // BUGFIX: previously used `static_cast<int64_t>(val.asNumber())`
                // as a truthiness check, which rejected the valid value 0.
                // Correct check: is a number AND has no fractional part AND
                // fits in int64_t.
                if (val.isNumber()) {
                    double d = val.asNumber();
                    typeOK = (d == std::trunc(d))
                          && (d >= static_cast<double>(std::numeric_limits<int64_t>::min()))
                          && (d <= static_cast<double>(std::numeric_limits<int64_t>::max()));
                }
                break;
            }
            case ColumnType::DOUBLE:
                typeOK = val.isNumber();
                break;
            case ColumnType::STRING:
                typeOK = val.isString();
                break;
            case ColumnType::BOOL:
                typeOK = val.isBool();
                break;
        }

        if (!typeOK) return Status::INVALID_TYPE;
    }

    return Status::OK;
}


// ============================================================
//  Section 4 — Serialization
// ============================================================
Json Record::toJson() const {
    Json envelope(Json::ObjectType{});
    envelope["__id__"] = static_cast<int>(id);
    envelope["__deleted__"] = deleted;
    envelope["data"] = data;
    return envelope;
}

Status Record::fromJson(const Json& envelope) {
    if (envelope.isNull()) return Status::PARSE_ERROR;

    id = static_cast<RecordID>(envelope["__id__"].asNumber());
    deleted = envelope["__deleted__"].asBool();
    data = envelope["data"];
    return Status::OK;
}

std::string Record::serialize() const {
    return toJson().dump();
}

Status Record::deserialize(const std::string& raw) {
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
//  Section 5 — Utility
// ============================================================
bool Record::isDeleted() const {
    return deleted;
}

void Record::markDeleted() {
    deleted = true;
}

RecordID Record::getID() const {
    return id;
}


// ============================================================
//  Section 6 — Operators
// ============================================================
bool Record::operator==(const Record& other) const {
    return id == other.id;
}

bool Record::operator!=(const Record& other) const {
    return id != other.id;
}

} // namespace MiniDB::Core
