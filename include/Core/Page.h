#pragma once

#include <string>

#include "../Common/Type.h"
#include "Record.h"
#include "../../libs/VectorPro/VectorPro.h"
#include "../../libs/CustomAllocators/PoolAllocator/PoolAllocator.h"

class Page {
    public:
    
    // Data Members
    PageID  id;
    bool    dirty = false;
    
    private:
    
    // Storage
    VectorPro<Record*>  records;
    PoolAllocator       pool;
    
    public:
    
    // Constructors & Destructor
    Page();
    explicit Page(PageID id);
    ~Page();
    
    Page(const Page&)            = delete;
    Page& operator=(const Page&) = delete;
    
    Page(Page&& other)             noexcept;
    Page& operator=(Page&& other)  noexcept;
    
    // Record Operations
    [[nodiscard]] Status         addRecord(const Record& record);
    [[nodiscard]] Record*        getRecord(RecordID id);
    [[nodiscard]] const Record*  getRecord(RecordID id) const;
    [[nodiscard]] Status         updateRecord(const Record& record);
    [[nodiscard]] Status         deleteRecord(RecordID id);
    
    // Page Management
    [[nodiscard]] Status compact();
    
    // Serialization 
    [[nodiscard]] std::string  serialize() const;
    [[nodiscard]] Status       deserialize(const std::string& raw);
    
    // Introspection 
    [[nodiscard]] bool         isFull() const noexcept;
    [[nodiscard]] bool         isDirty() const noexcept;
    [[nodiscard]] bool         isEmpty() const noexcept;
    [[nodiscard]] std::size_t  recordCount() const noexcept;
    [[nodiscard]] std::size_t  freeSlots() const noexcept;
    [[nodiscard]] PageID       getID() const noexcept;
    
};
