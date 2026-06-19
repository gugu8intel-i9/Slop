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
    std::cout << "--- [C++ Benchmark: Counting to 1 Billion] ---" << std::endl;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long long limit = 1000000000;
    long long count = 0;
    while (count < limit) {
        count++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Execution Time: %.6f seconds\n", elapsed);

    std::cout << "Final Count:" << std::endl;
    std::cout << count << std::endl;

    print_ram_usage();
    return 0;
}
