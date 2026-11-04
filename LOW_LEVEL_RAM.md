# Low-level RAM control

`use "ram"` gives you direct control over memory: ask the operating system
for raw pages, set what they may be used for, lock them in RAM, and carve
small pieces out of a block quickly. You do not need it for ordinary
programs. Use it when you want memory the normal allocator does not manage.

```slop
use "ram"

size = ram_page_size() * 4
p = ram_alloc(size)                 # zero-filled, page-aligned, from the OS
ram_store_f64(p, 2.5)
print(ram_load_f64(p))              # 2.5

ram_protect(p, size, "r")           # read-only now
ram_protect(p, size, "rw")          # and back to read-write

a = ram_arena(4096)                 # a fast bump allocator in one block
x = ram_arena_take(a, 16, 8)        # 16 bytes, 8-byte aligned; 0 if full
ram_arena_free(a)

ram_free(p, size)                   # give the pages back (same size as asked)
```

Memory from `ram_alloc` is not touched by the normal allocator, so it is
never moved or freed for you. Free it with `ram_free` and the same byte count
you asked for.

## Functions

| Function | What it does |
|---|---|
| `ram_page_size()` | the size of a page in bytes (4096, or 16384 on Apple Silicon) |
| `ram_total()` | bytes of physical RAM |
| `ram_available()` | bytes that can be used now without swapping |
| `ram_alloc(bytes)` | zero-filled, page-aligned memory from the OS; 0 on failure |
| `ram_free(addr, bytes)` | gives memory from `ram_alloc` back |
| `ram_alloc_huge(bytes)` | like `ram_alloc`, asking for huge pages; never fails just for lack of them |
| `ram_protect(addr, bytes, mode)` | `mode` is `"none"`, `"r"`, `"rw"`, `"rx"` or `"rwx"` |
| `ram_lock(addr, bytes)` / `ram_unlock(addr, bytes)` | keep pages in RAM; may be refused (returns false) |
| `ram_advise(addr, bytes, how)` | a hint: `"normal"`, `"random"`, `"sequential"`, `"willneed"`, `"dontneed"`, `"hugepage"` |
| `ram_fill(addr, bytes, byte)` | set every byte to `byte` (0 to 255) |
| `ram_equal(a, b, bytes)` | true if the two blocks hold the same bytes |
| `ram_load_f64(addr)`, `ram_store_f64(addr, v)` | 64-bit float in memory |
| `ram_load_f32(addr)`, `ram_store_f32(addr, v)` | 32-bit float in memory |
| `ram_arena(bytes)` | a new arena (its handle is an address; 0 on failure) |
| `ram_arena_take(arena, bytes, align)` | bytes from the arena at a multiple of `align`; 0 if full |
| `ram_arena_used(arena)` | bytes taken so far |
| `ram_arena_reset(arena)` | forget what was taken (the memory is kept, not cleared) |
| `ram_arena_free(arena)` | give the whole arena back |

Also built in, from before this module: `ram_copy(dst, src, bytes)`,
`ram_zero(dst, bytes)`, and `unsafe_load8/16/32/64` and
`unsafe_store8/16/32/64` for reading and writing words at an address.

Arena memory is not cleared when you take it; only `ram_alloc` memory is
zero-filled.

## Which systems support what

| Function | Linux (x86-64, ARM64) | macOS (Intel, Apple Silicon) | Windows (x86-64, ARM64) |
|---|---|---|---|
| page size, total, available | yes (`MemAvailable`) | yes (`hw.memsize`, free pages) | yes |
| `ram_alloc` / `ram_free` | `mmap` / `munmap` | `mmap` / `munmap` | `VirtualAlloc` / `VirtualFree` |
| `ram_alloc_huge` | `MAP_HUGETLB` for sizes that are whole 2 MB pages, otherwise normal pages with transparent huge pages requested | normal pages | normal pages (large pages need a privilege) |
| `ram_protect` | yes | yes | yes (`VirtualProtect`) |
| `ram_lock` / `ram_unlock` | `mlock` / `munlock` | `mlock` / `munlock` | `VirtualLock` / `VirtualUnlock` |
| `ram_advise` `normal`, `random`, `sequential`, `willneed`, `dontneed` | yes (`madvise`) | yes (`madvise`) | `willneed` and `dontneed` only; the others return false |
| `ram_advise` `hugepage` | yes | returns false | returns false |
| fill, equal, typed access, arena | yes | yes | yes |

Notes:

- `ram_free` needs the same byte count that was asked of `ram_alloc`.
- On Windows, `dontneed` discards the contents of the memory.
- Locking needs room under the system's limits (on Linux, `RLIMIT_MEMLOCK`).
  Check the result instead of assuming it worked.
- Reading or writing memory that is protected as `"none"` (or `"r"` when
  writing) crashes the program.
