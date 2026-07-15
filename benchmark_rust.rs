use std::fs;
use std::time::Instant;

fn print_ram_usage() {
    if let Ok(status) = fs::read_to_string("/proc/self/status") {
        for line in status.lines() {
            if line.starts_with("VmRSS:") {
                println!("{}", line);
                break;
            }
        }
    }
}

fn main() {
    println!("--- [Rust Benchmark: Dead 10 Billion Counter Loop] ---");
    println!("counter is intentionally unused after the loop so optimized LLVM can delete it");

    let start = Instant::now();

    let limit: i64 = 10_000_000_000; // 10 Billion
    let mut counter: i64 = 0;
    while counter < limit {
        counter += 1;
    }

    // Do not print or otherwise consume counter. In optimized builds the loop
    // has no observable side effects, so it should be removed completely.
    let elapsed = start.elapsed().as_secs_f64();
    println!("Execution Time: {:.6} seconds", elapsed);

    print_ram_usage();
}
