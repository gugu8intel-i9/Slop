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
    long long elapsed_ns = (long long)(end.tv_sec - start.tv_sec) * 1000000000LL + (long long)(end.tv_nsec - start.tv_nsec);
    double elapsed = elapsed_ns / 1000000000.0;
    printf("Measured Loop Region: %.9f seconds (%lld ns)\n", elapsed, elapsed_ns);

    print_ram_usage();
    return 0;
}
