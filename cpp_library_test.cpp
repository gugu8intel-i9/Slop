#include "slop_bridge.hpp"
#include <iostream>

// Define global call depth variable required by the runtime
int slop_arena_depth = 0;

// Simple function that utilizes Slop's SEAA memory arena for high performance
void run_slop_computation(SlopArena* arena) {
    // 1. Create Slop strings inside the arena (using O(1) allocation)
    SlopString greet = slop::copy_to_slop_string(arena, "Hello from C++ via the Slop High-Performance Bridge!");
    
    // 2. Print it using the Slop runtime
    slop_print_string(greet);

    // 3. Create a dynamic vector and bridge it to Slop Array zero-copy
    std::vector<int64_t> numbers = {10, 20, 30, 40, 50};
    SlopArray slop_arr = slop::to_slop_array(arena, numbers);

    // 4. Compute sum using Slop-style pointer access
    int64_t sum = 0;
    for (size_t i = 0; i < slop_arr.length; i++) {
        sum += *(int64_t*)slop_arr.data[i];
    }
    
    std::cout << "Sum calculated on Slop Arena: " << sum << std::endl;
}

int main() {
    std::cout << "=== [C++ & Slop High-Performance Bridge Test] ===" << std::endl;

    // Use RAII Scope to manage the Arena/Bucket stack lifecycles automatically
    slop::run_native([](SlopArena* arena) {
        run_slop_computation(arena);
    });

    std::cout << "=== [Execution Completed. Memory Wiped instantly] ===" << std::endl;
    return 0;
}
