#pragma once

#include "../../include/Core/Table.h"
#include "../../include/Core/Record.h"
#include "../../libs/VectorPro/VectorPro.h"
#include "../../include/Common/Type.h"

// Builds a simple "employees" table schema for use across tests
inline VectorPro<ColumnDef> makeEmployeeSchema() {
    VectorPro<ColumnDef> schema;
    schema.push_back({"name",   ColumnType::STRING, false});
    schema.push_back({"salary", ColumnType::DOUBLE, false});
    schema.push_back({"dept",   ColumnType::STRING, true});
    return schema;
}

// Builds a ready-to-use Table with the employee schema
inline Table makeEmployeeTable() {
    return Table("employees", 0, makeEmployeeSchema());
}

// Builds a simple test record for an employee
inline Record makeEmployeeRecord(RecordID id, const std::string& name, double salary) {
    Json data(Json::ObjectType{});
    data["name"]   = name;
    data["salary"] = salary;
    return Record(id, data);
}

