#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

void print_ram_usage() {
    std::ifstream statm("/proc/self/status");
    std::string line;
    while (std::getline(statm, line)) {
        if (line.find("VmRSS:") != std::string::npos) {
            std::cout << line << std::endl;
            break;
        }
    }
}

int main() {
    std::cout << "--- [C++ Benchmark: Dead 10 Billion Counter Loop] ---" << std::endl;
    std::cout << "counter is intentionally unused after the loop so GCC/Clang can delete it" << std::endl;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long long limit = 10000000000LL; // 10 Billion
    long long counter = 0;
    while (counter < limit) {
        counter++;
    }

    // Do not print or otherwise consume counter. In optimized builds the loop
    // has no observable side effects, so it should be removed completely.
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Execution Time: %.6f seconds\n", elapsed);

    print_ram_usage();
    return 0;
}
