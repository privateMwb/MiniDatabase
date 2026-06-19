// Students Example
//
// Demonstrates a realistic "academic" use case for MiniDatabase:
// managing a class roster with course-based filtering, grade
// aggregation, and JSON export/import via the Serializer.
//
// Covers:
// - Creating a database and table
// - Inserting multiple records
// - Filtering by course
// - Finding the highest grade via the max aggregate
// - Exporting a table to JSON with Serializer
// - Importing a table from JSON into a fresh database

#include "../include/Core/Database.h"
#include "../include/Engine/QueryEngine.h"
#include "../include/Engine/Serializer.h"
#include "../libs/CustomAllocators/ArenaAllocator/ArenaAllocator.h"
#include "../libs/VectorPro/VectorPro.h"

#include <iostream>
#include <string>

// Builds the students table schema.
static VectorPro<ColumnDef> makeStudentSchema() {
    VectorPro<ColumnDef> schema;

    schema.push_back({ "name",   ColumnType::STRING, false });
    schema.push_back({ "age",    ColumnType::INT,    false });
    schema.push_back({ "course", ColumnType::STRING, false });
    schema.push_back({ "grade",  ColumnType::DOUBLE, false });

    return schema;
}

// Builds a single student record.
static Record makeStudent(
    RecordID            id,
    const std::string&  name,
    int                 age,
    const std::string&  course,
    double              grade)
{
    Json fields(Json::ObjectType{});

    fields["name"]   = name;
    fields["age"]    = age;
    fields["course"] = course;
    fields["grade"]  = grade;

    return Record(id, fields);
}

// Prints a single record's relevant fields in a readable line.
static void printStudent(const Record& r) {
    std::cout << "[" << r.getID() << "] "
               << r.getField("name").asString()
               << " (" << r.getField("age").asNumber() << "y/o) - "
               << r.getField("course").asString()
               << " (" << r.getField("grade").asNumber() << ")\n";
}

int main() {
    std::cout << "=== MiniDatabase: Students Example ===\n\n";

    // 1. Create database and table
    Database db("ClassDB");
    Status createStatus = db.createTable("students", makeStudentSchema());
    if (createStatus != Status::OK) {
        std::cout << "Failed to create table.\n";
        return 1;
    }

    // 2. Insert the students
    Table* students = db.getTable("students");
    (void)students->insertRecord(makeStudent(1, "Frank", 20, "Computer Science", 88));
    (void)students->insertRecord(makeStudent(2, "Grace", 22, "Computer Science", 95));
    (void)students->insertRecord(makeStudent(3, "Henry", 19, "Mathematics",      76));
    (void)students->insertRecord(makeStudent(4, "Ivy",   21, "Mathematics",      91));
    (void)students->insertRecord(makeStudent(5, "Jack",  23, "Computer Science", 67));

    std::cout << "Inserted " << students->recordCount() << " students.\n\n";

    // 3. Filter by course == "Computer Science"
    ArenaAllocator arena(DBConstants::ARENA_SIZE);
    QueryEngine    qe(arena);

    FilterPredicate comsciFilter{"course", Op::EQ, Json("Computer Science")};
    VectorPro<FilterPredicate*> coursePredicate;
    coursePredicate.push_back(&comsciFilter);

    QueryResult coms = qe.select(*students, coursePredicate);

    std::cout << "Computer Science Students:\n";
    for (const Record& r : coms.records) {
        printStudent(r);
    }
    std::cout << "\n";

    // 4. Find max grade across all students
    VectorPro<FilterPredicate*> noPredicates;
    double maxGrade = qe.max(*students, "grade", noPredicates);

    FilterPredicate gradeFilter{"grade", Op::EQ, Json(maxGrade)};
    VectorPro<FilterPredicate*> gradePredicate;
    gradePredicate.push_back(&gradeFilter);

    QueryResult topStudents = qe.select(*students, gradePredicate);

    std::cout << "Max Grade: " << maxGrade << "\n";
    std::cout << "Students:\n";
    for (const Record& r : topStudents.records) {
        std::cout << r.getField("name").asString() << "\n";
    }
    std::cout << "\n";

    // 5. Export the table to JSON using the Serializer
    const std::string path = "examples/exported/students.json";
    Status saveStatus = Serializer::exportTableToFile(*students, path);
    std::cout << "Save to disk: " << (saveStatus == Status::OK ? "processed" : "failed") << "\n\n";

    // 6. Import that same file into a fresh database via the Serializer
    Database restored("");
    Status restoredCreateStatus = restored.createTable("students", makeStudentSchema());
    if (restoredCreateStatus != Status::OK) {
        std::cout << "Failed to create table.\n";
        return 1;
    }

    Table* newTable = restored.getTable("students");
    if (!newTable) {
        std::cout << "Failed to retrieve table.\n";
        return 1;
    }

    Status loadStatus = Serializer::importTableFromFile(*newTable, path);
    std::cout << "Load from disk: " << (loadStatus == Status::OK ? "processed" : "failed") << "\n\n";

    std::cout << "Restored database has " << restored.tableCount() << " table(s) "
              << "and " << restored.recordCount() << " record(s).\n";

    return 0;
}