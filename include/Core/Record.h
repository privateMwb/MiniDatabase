#pragma once

#include <string>

#include "../Common/Type.h"
#include "../../libs/JsonParser/Json.h"
#include "../../libs/VectorPro/VectorPro.h"

class Record {
    public:
    
    // Data Members
    RecordID  id;
    Json      data;
    bool      deleted = false;
    
    // Constructors
    Record();
    explicit Record(RecordID id);
    Record(RecordID id, Json data);
    
    // Field Access
    [[nodiscard]] Status  setField(const FieldName& key, const Json& value);
    [[nodiscard]] Json    getField(const FieldName& key) const;
    [[nodiscard]] bool    hasField(const FieldName& key) const;
    [[nodiscard]] Status  removeField(const FieldName& key);
    
    // Schema Validation
    [[nodiscard]] Status validate(const VectorPro<ColumnDef>& schema) const;
    
    // Serialization 
    [[nodiscard]] std::string  serialize() const;
    [[nodiscard]] Status       deserialize(const std::string& raw);
    
    // Utility
    [[nodiscard]] bool      isDeleted() const;
    void                    markDeleted();
    [[nodiscard]] RecordID  getID() const;
    
    // Operators
    [[nodiscard]] bool operator==(const Record& other) const;
    [[nodiscard]] bool operator!=(const Record& other) const;
    
};
