#include <iostream>

void run_record_tests();
void run_page_tests();      // added as each file is written
void run_table_tests();
void run_database_tests();
void run_query_tests();
void run_storage_tests();
// void run_serializer_tests();

int main() {
    run_record_tests();
    run_page_tests();
    run_table_tests();
    run_database_tests();
    run_query_tests();
    run_storage_tests();
    
    std::cout << "\nAll tests completed.\n";
    return 0;
}
