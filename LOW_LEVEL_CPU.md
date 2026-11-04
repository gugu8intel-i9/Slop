# Low-level CPU control: `use "cpu"`

The `cpu` module tells a program what processor it runs on, and lets a thread
choose its core and priority. It is optional: a program that does not write
`use "cpu"` does not get any of it. Every answer is best effort. Unknown text
is `""`, unknown numbers are `0` or `-1`, and a yes/no answer that cannot be
checked is `false`.

## Example

```slop
use "cpu"

print(cpu_vendor(), cpu_logical_cores(), "cores")
print("AVX2:", cpu_has("avx2"))
print("cache line:", cpu_cache_line(), "bytes")

# run this thread on core 0 only, then let it roam again
if cpu_pin(0) {
    print("now on core", cpu_current())
    cpu_unpin()
}

start = cpu_thread_time_ns()
# ... work ...
print("this thread used", cpu_thread_time_ns() - start, "ns")
```

## Functions

| Function | What it gives |
|---|---|
| `cpu_vendor() -> string` | `"GenuineIntel"`, `"AuthenticAMD"`, `"ARM"`, `"Apple"`, ... or `""` |
| `cpu_model() -> string` | The processor's name, or `""` |
| `cpu_logical_cores() -> int` | Hardware threads this program may use |
| `cpu_physical_cores() -> int` | Physical cores (the logical count if not known) |
| `cpu_has(feature) -> bool` | Feature test, see the names below |
| `cpu_cache_line() -> int` | Cache line size in bytes (64 if not known) |
| `cpu_cache_size(level) -> int` | Bytes of cache level 1 (data), 2 or 3; 0 if not known |
| `cpu_pin(core) -> bool` | Pin the calling thread to one logical core |
| `cpu_unpin() -> bool` | Let the calling thread run on any core again |
| `cpu_current() -> int` | The core the calling thread runs on now; -1 if not known |
| `cpu_priority(level) -> bool` | Set the calling thread's priority: `"low"`, `"normal"`, `"high"` |
| `cpu_yield()` | Give up the rest of this thread's time slice |
| `cpu_thread_time_ns() -> int` | CPU time used by the calling thread, in nanoseconds |

Feature names for `cpu_has` (lower case; anything else is `false`):

- x86: `"sse2"`, `"sse4.2"`, `"avx"`, `"avx2"`, `"avx512f"`, `"fma"`, `"aes"`, `"sha"`, `"popcnt"`, `"bmi2"`
- ARM: `"neon"` (also `"asimd"`), `"aes"`, `"sha2"`, `"crc32"`, `"atomics"`, `"sve"`

`cpu_pin` and `cpu_priority` can fail without the right privileges (raising a
priority, for one), and then they return `false`. `cpu_pin` sets the thread's
affinity, and the system may move the thread at its next scheduling point, so
call `cpu_yield()` after `cpu_pin` before you rely on `cpu_current()`.

## What each system supports

| Function | Linux (x86-64, ARM64) | macOS (Intel, Apple Silicon) | Windows (x86-64, ARM64) |
|---|---|---|---|
| vendor | `vendor_id`; ARM from the implementer code | `machdep.cpu.vendor`; `"Apple"` on Apple Silicon | from the processor identifier; `""` if absent |
| model | `model name` (or `Model` on ARM boards) | brand string | the processor identifier, e.g. `Intel64 Family 6 Model 158 Stepping 10` |
| logical / physical cores | yes / distinct core ids in `/proc/cpuinfo` | yes / `hw.physicalcpu` | yes / `GetLogicalProcessorInformation` |
| `cpu_has` | `/proc/cpuinfo`; NEON is always true on ARM64 | `hw.optional.*` sysctls | `IsProcessorFeaturePresent`; x86 `aes`, `sha`, `fma`, `popcnt`, `bmi2` are not exposed, so `false` |
| cache line and sizes | `/sys/devices/system/cpu/cpu0/cache` | `hw.cachelinesize` (128 on Apple Silicon), `hw.l*cachesize` | `GetLogicalProcessorInformation` |
| pin / unpin | yes (`sched_setaffinity`) | no: `false` | yes (`SetThreadAffinityMask`) |
| current core | yes (`getcpu`) | `-1` | yes (`GetCurrentProcessorNumber`) |
| priority | nice values 19 / 0 / -10 | `setpriority` on the thread | thread priority lowest / normal / highest |
| yield | yes | yes | yes (`SwitchToThread`) |
| thread time | yes (`CLOCK_THREAD_CPUTIME_ID`) | yes (`clock_gettime_nsec_np`; 0 if not known) | yes (`GetThreadTimes`) |

Notes:

- `cpu_has` reads files or system calls on every call. Keep the result in a
  variable when you need it in a loop.
- The module is not for ordinary programs. Pinning threads or reading cache
  sizes is useful for benchmarks, schedulers and tuning, and the numbers
  describe the machine the program runs on, not what the program may rely on.
