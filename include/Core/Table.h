#pragma once

#include <string>

#include "../Common/Type.h"
#include "Record.h"
#include "Page.h"

#include "../../libs/VectorPro/VectorPro.h"
#include "../../libs/HashMap/HashMap.h"

class Table {
public:

	// Data Members
	std::string  name;
	TableID      id;

private:

	// Schema
	VectorPro<ColumnDef> schema;

	// Storage
	VectorPro<Page*>           pages;
	HashMap<RecordID, PageID>  index;

	// Identity
	bool dirty = false;

public:

	// Constructors & Destructor
	Table(std::string name, TableID id, VectorPro<ColumnDef> schema);
	~Table();

	Table(const Table&)              = delete;
	Table& operator=(const Table&&)  = delete;

	Table(Table&& other)             noexcept;
	Table& operator=(Table&& other)  noexcept;

	// CRUD
	[[nodiscard]] Status insertRecord(const Record& record);
	[[nodiscard]] Status getRecord(RecordID id, Record& out) const;
	[[nodiscard]] Status updateRecord(const Record& record);
	[[nodiscard]] Status deleteRecord(RecordID id);

	// Schema
	[[nodiscard]] const VectorPro<ColumnDef>& getSchema() const noexcept;
	[[nodiscard]] bool                       hasColumn(const FieldName& name) const noexcept;

	// Page Management
	[[nodiscard]] Status compact();
	[[nodiscard]] const VectorPro<Page*>& getPages() const noexcept;

	// Index
	[[nodiscard]] Status rebuildIndex();

	// Serialization
	[[nodiscard]] std::string serialize() const;
	[[nodiscard]] Status deserialize(const std::string& raw);

	// Introspection
	[[nodiscard]] std::string  getName()      const noexcept;
	[[nodiscard]] TableID      getID()        const noexcept;
	[[nodiscard]] std::size_t  recordCount()  const noexcept;
	[[nodiscard]] std::size_t  pageCount()    const noexcept;
	[[nodiscard]] bool         isDirty()      const noexcept;
	[[nodiscard]] bool         isEmpty()      const noexcept;

private:

	// Helper
	[[nodiscard]] Page*        findPageWithSlot()       noexcept;
	[[nodiscard]] Page*        findPageByID(PageID id)  noexcept;
	[[nodiscard]] const Page*  findPageByID(PageID id)  const noexcept;
	[[nodiscard]] PageID       nextPageID()             const noexcept;
};

