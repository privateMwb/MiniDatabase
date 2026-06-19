#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>

void run_insert_benchmarks();
void run_query_benchmarks();
void run_storage_benchmarks();
void run_serializer_benchmarks();

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    run_insert_benchmarks();
    run_query_benchmarks();
    run_storage_benchmarks();
    run_serializer_benchmarks();

    return 0;
}