const std = @import("std");

fn printRAMUsage() !void {
    var file = std.fs.openFileAbsolute("/proc/self/status", .{}) catch return;
    defer file.close();

    var buf: [8192]u8 = undefined;
    const n = try file.readAll(&buf);
    var lines = std.mem.splitScalar(u8, buf[0..n], '\n');
    const stdout = std.io.getStdOut().writer();
    while (lines.next()) |line| {
        if (std.mem.startsWith(u8, line, "VmRSS:")) {
            try stdout.print("{s}\n", .{line});
            break;
        }
    }
}

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();
    try stdout.print("--- [Zig Benchmark: Dead 10 Billion Counter Loop] ---\n", .{});
    try stdout.print("counter is intentionally unused after the loop so optimized Zig/LLVM can delete it\n", .{});

    const start = std.time.nanoTimestamp();

    const limit: i64 = 10_000_000_000; // 10 Billion
    var counter: i64 = 0;
    while (counter < limit) : (counter += 1) {}

    // Do not print or otherwise consume counter. In optimized builds the loop
    // has no observable side effects, so it may be removed completely.
    const elapsed_ns = std.time.nanoTimestamp() - start;
    const elapsed = @as(f64, @floatFromInt(elapsed_ns)) / 1_000_000_000.0;
    try stdout.print("Measured Loop Region: {d:.9} seconds ({d} ns)\n", .{ elapsed, elapsed_ns });

    try printRAMUsage();
}
