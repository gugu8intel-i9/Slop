use slop_bridge::{run_native, slop_print_string, SlopScope};

fn main() {
    println!("=== [Rust & Slop High-Performance Bridge Test] ===");

    // Run safe, native code on Slop's SEAA memory arena stack
    run_native(|scope: &SlopScope| {
        // 1. Allocate a string in the Slop Arena
        let slop_str = scope.create_string("Hello from Rust via the Slop FFI Bridge!");
        
        // 2. Call Slop runtime print
        unsafe {
            slop_print_string(slop_str);
        }

        // 3. Convert Slop string back to a Rust slice (zero-copy)
        let rust_slice = scope.to_rust_str(slop_str);
        println!("Rust read zero-copy string slice: '{}'", rust_slice);

        // 4. Create an integer array on the Slop Arena
        let numbers = vec![1, 2, 3, 4, 5];
        let slop_array = scope.create_int_array(&numbers);
        println!("Slop Array created with length: {}", slop_array.length);
    });

    println!("=== [Execution Completed. Rust Scope Dropped, Arena Reclaimed] ===");
}
