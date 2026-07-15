package main

import (
    "bufio"
    "fmt"
    "os"
    "strings"
    "time"
)

func printRAMUsage() {
    f, err := os.Open("/proc/self/status")
    if err != nil {
        return
    }
    defer f.Close()

    scanner := bufio.NewScanner(f)
    for scanner.Scan() {
        line := scanner.Text()
        if strings.HasPrefix(line, "VmRSS:") {
            fmt.Println(line)
            break
        }
    }
}

func main() {
    fmt.Println("--- [Go Benchmark: Dead 10 Billion Counter Loop] ---")
    fmt.Println("counter is intentionally unused after the loop so optimized compilers can delete it")

    start := time.Now()

    const limit int64 = 10_000_000_000 // 10 Billion
    var counter int64 = 0
    for counter < limit {
        counter++
    }

    // Do not print or otherwise consume counter. In optimized builds the loop
    // has no observable side effects, so it can be removed completely.
    elapsed := time.Since(start)
    fmt.Printf("Measured Loop Region: %.9f seconds (%d ns)\n", elapsed.Seconds(), elapsed.Nanoseconds())

    printRAMUsage()
}
