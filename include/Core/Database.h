#pragma once

#include <string>

#include "../Common/Type.h"
#include "Table.h"

#include "../../libs/VectorPro/VectorPro.h"
#include "../../libs/HashMap/HashMap.h"

class Database {
public:

	// Data Members
	std::string name;

private:

	// Storage
	VectorPro<Table*>              tables;
	HashMap<std::string, TableID>  index;

	// Identity
	bool dirty = false;

public:

	// Constructors & Destructors
	explicit Database(std::string name);
	~Database();

	Database(const Database&)             = delete;
	Database& operator=(const Database&)  = delete;

	Database(Database&& other)             noexcept;
	Database& operator=(Database&& other)  noexcept;

	// Table Management
	[[nodiscard]] Status  createTable(const std::string& name, VectorPro<ColumnDef> schema);
	[[nodiscard]] Status  dropTable(const std::string& name);
	[[nodiscard]] Table*  getTable(const std::string& name);
	[[nodiscard]] const   Table* getTable(const std::string& name) const;
	[[nodiscard]] bool    hasTable(const std::string& name) const noexcept;

	// Database Operations
	[[nodiscard]] Status save(const std::string& path) const;
	[[nodiscard]] Status load(const std::string& path);
	[[nodiscard]] Status compact();

	// Introspection
	[[nodiscard]] std::string               getName()      const noexcept;
	[[nodiscard]] std::size_t               tableCount()   const noexcept;
	[[nodiscard]] std::size_t               recordCount()  const noexcept;
	[[nodiscard]] bool                      isDirty()      const noexcept;
	[[nodiscard]] bool                      isEmpty()      const noexcept;
	[[nodiscard]] const VectorPro<Table*>&  getTables()    const noexcept;

private:

	// Helper
	[[nodiscard]] TableID nextTableID() const noexcept;
};
