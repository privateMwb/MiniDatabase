#include "../../include/Core/Record.h"

// Constructors
Record::Record()
	: id(DBConstants::INVALID_RECORD_ID)
        , data(Json::ObjectType{}) {}

Record::Record(RecordID id)
	: id(id)
        , data(Json::ObjectType{}) {}

Record::Record(RecordID id, Json data)
	: id(id)
	, data(std::move(data)) {}

// Field Access
Status Record::setField(const FieldName& key, const Json& value) {
	if(key.empty()) return Status::INVALID_SCHEMA;
	data[key] = value;
	return Status::OK;
}

Json Record::getField(const FieldName& key) const {
	if(!data.contains(key)) return Json{};
	return data[key];
}

bool Record::hasField(const FieldName& key) const {
	return data.contains(key);
}

Status Record::removeField(const FieldName& key) {
	if(!data.contains(key)) return Status::NOT_FOUND;
	data.asObject().erase(key);
	return Status::OK;
}

// Schema Validation
Status Record::validate(const VectorPro<ColumnDef>& schema) const {
	for(const auto& col : schema) {
		if(!data.contains(col.name)) {
			if(!col.nullable) return Status::INVALID_SCHEMA;
			continue;
		}

		const Json& val = data[col.name];
		bool typeOK = false;

		switch(col.type) {
		case ColumnType::INT:
			typeOK = (val.isNumber() && static_cast<int64_t>(val.asNumber()));
			break;
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
		
		if(!typeOK) return Status::INVALID_TYPE;
	}

	return Status::OK;
}

// Serialization
std::string Record::serialize() const {
    Json envelope;
    envelope["__id___"] = static_cast<int>(id);
    envelope["__deleted__"] = deleted;
    envelope["data"] = data;
    return envelope.dump();
}

Status Record::deserialize(const std::string& raw) {
    Json envelope = Json::parse(raw);
    if(envelope.isNull()) return Status::PARSE_ERROR;
    
    id = static_cast<RecordID>(envelope["__id__"].asNumber());
    deleted = envelope["__deleted__"].asBool();
    data = envelope["data"];
    return Status::OK;
}

// Utility 
bool Record::isDeleted() const {
    return deleted;
}

void Record::markDeleted() {
    deleted = true;
}

RecordID Record::getID() const {
    return id;
}

// Operators
bool Record::operator==(const Record& other) const {
    return id == other.id;
}

bool Record::operator!=(const Record& other) const {
    return id != other.id;
}







