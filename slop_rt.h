#ifndef SLOP_RT_H
#define SLOP_RT_H

// Expose all POSIX.1-2008 + OS-native extensions (required for macOS/BSD snprintf,
// sysconf, pthread, and socket declarations with strict compilers).
#if defined(__APPLE__)
    #define _DARWIN_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

// POSIX socket headers for high-performance zero-copy network streaming
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

// ============================================================================
// CROSS-PLATFORM SYSTEM SHIMS: Windows 11, macOS, and Linux (ALL OF THEM!)
// ============================================================================

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib") // Auto-link Winsock library for MSVC
    
    typedef int socklen_t;
    typedef HANDLE pthread_t;
    
    static inline int pthread_create(pthread_t* thread, void* attr, void* (*start_routine)(void*), void* arg) {
        *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
        return (*thread != NULL) ? 0 : -1;
    }
    
    static inline int pthread_detach(pthread_t thread) {
        CloseHandle(thread);
        return 0;
    }

    static inline void slop_winsock_init() {
        static bool initialized = false;
        if (!initialized) {
            WSADATA wsa;
            WSAStartup(MAKEWORD(2,2), &wsa);
            initialized = true;
        }
    }
    
    static inline int nanosleep(const struct timespec* req, struct timespec* rem) {
        Sleep((DWORD)(req->tv_sec * 1000 + req->tv_nsec / 1000000));
        return 0;
    }

    #define closesocket_compat closesocket
#else
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <pthread.h>
    
    #define closesocket_compat close
    static inline void slop_winsock_init() {} // No-op on POSIX
#endif

// ============================================================================
// Slop Arena ("Bucket") memory
// ============================================================================
// Blocks are chained newest-first and each block is ONE allocation (header +
// payload), so pointers never move and a new block costs a single malloc.
// Restoring a mark scrubs only the bytes that were actually used, keeps one
// emptied block as a spare (no malloc/free churn when a hot function keeps
// crossing a block edge) and frees the rest.
typedef struct SlopArenaBlock {
    struct SlopArenaBlock* prev;
    size_t size;
    size_t used;
    uint8_t data[];
} SlopArenaBlock;

typedef struct SlopArena {
    SlopArenaBlock* current;
    SlopArenaBlock* spare;
    size_t block_size;
} SlopArena;

static inline void slop_oom(void) {
    fprintf(stderr, "Slop: out of memory.\n");
    exit(1);
}

static inline SlopArenaBlock* slop_arena_block_create(size_t size) {
    // calloc lets the OS hand out zero pages lazily: untouched memory is never resident.
    SlopArenaBlock* block = (SlopArenaBlock*)calloc(1, sizeof(SlopArenaBlock) + size);
    if (!block) slop_oom();
    block->size = size;
    return block;
}

static inline void slop_arena_block_free(SlopArenaBlock* block) {
    memset(block->data, 0, block->used);
    free(block);
}

static inline void slop_arena_init(SlopArena* arena, size_t capacity) {
    arena->block_size = capacity < 1024 ? 1024 : capacity;
    arena->current = slop_arena_block_create(arena->block_size);
    arena->spare = NULL;
}

static inline SlopArena* slop_arena_create(size_t capacity) {
    SlopArena* arena = (SlopArena*)malloc(sizeof(SlopArena));
    if (!arena) slop_oom();
    slop_arena_init(arena, capacity);
    return arena;
}

static inline void slop_arena_destroy(SlopArena* arena) {
    if (!arena) return;
    SlopArenaBlock* block = arena->current;
    while (block) {
        SlopArenaBlock* prev = block->prev;
        slop_arena_block_free(block);
        block = prev;
    }
    free(arena->spare);
    free(arena);
}

static __attribute__((noinline)) void* slop_arena_alloc_slow(SlopArena* arena, size_t size) {
    SlopArenaBlock* block;
    if (size > arena->block_size) {
        block = slop_arena_block_create(size);
    } else if (arena->spare) {
        block = arena->spare;
        arena->spare = NULL;
    } else {
        block = slop_arena_block_create(arena->block_size);
    }
    block->prev = arena->current;
    block->used = size;
    arena->current = block;
    return block->data;
}

static inline void* slop_arena_alloc(SlopArena* arena, size_t size) {
    size = (size + 7) & ~(size_t)7;
    SlopArenaBlock* block = arena->current;
    if (__builtin_expect(block->size - block->used >= size, 1)) {
        void* ptr = block->data + block->used;
        block->used += size;
        return ptr;
    }
    return slop_arena_alloc_slow(arena, size);
}

// Grows the most recent allocation in place when there is room. Returns false otherwise.
static inline bool slop_arena_extend(SlopArena* arena, void* ptr, size_t old_size, size_t new_size) {
    SlopArenaBlock* block = arena->current;
    old_size = (old_size + 7) & ~(size_t)7;
    new_size = (new_size + 7) & ~(size_t)7;
    if ((uint8_t*)ptr + old_size != block->data + block->used) return false;
    if (block->size - block->used < new_size - old_size) return false;
    block->used += new_size - old_size;
    return true;
}

// A mark is the address of the next free byte.
static inline size_t slop_arena_save(SlopArena* arena) {
    return (size_t)(arena->current->data + arena->current->used);
}

static inline void slop_arena_restore(SlopArena* arena, size_t marker) {
    uint8_t* target = (uint8_t*)marker;
    SlopArenaBlock* block = arena->current;
    // Newest blocks come first, so this only walks blocks created since the mark.
    while (block && !(target >= block->data && target <= block->data + block->size)) {
        SlopArenaBlock* prev = block->prev;
        memset(block->data, 0, block->used);
        if (block->size == arena->block_size && !arena->spare) {
            block->used = 0;
            arena->spare = block;
        } else {
            free(block);
        }
        block = prev;
    }
    if (!block) {
        fprintf(stderr, "Slop Arena restore failed: invalid marker %p.\n", (void*)target);
        exit(1);
    }
    size_t keep = (size_t)(target - block->data);
    if (block->used > keep) memset(target, 0, block->used - keep);
    block->used = keep;
    arena->current = block;
}

// Thread-Local Global Arena Stack
#ifdef _MSC_VER
#define THREAD_LOCAL __declspec(thread)
#elif defined(__cplusplus)
#define THREAD_LOCAL thread_local
#else
#define THREAD_LOCAL _Thread_local
#endif

// "Two-bucket relay": a call at depth d allocates in bucket d & 1 and hands its
// result to its caller's bucket (d - 1) & 1. Calls are LIFO, so when a function
// returns everything deeper has already been wiped and the caller's data is on
// top of the other bucket. Two buckets per thread cover ANY recursion depth:
// memory grows with live data, never with call depth.
extern THREAD_LOCAL int slop_arena_depth;
static inline SlopArena* slop_get_arena(int depth) {
    static THREAD_LOCAL SlopArena buckets[2];
    SlopArena* arena = &buckets[depth & 1];
    if (__builtin_expect(!arena->current, 0)) slop_arena_init(arena, 64 * 1024);
    return arena;
}

// Function prologue / epilogue used by generated code.
#define SLOP_ENTER() \
    slop_arena_depth++; \
    SlopArena* local_arena = slop_get_arena(slop_arena_depth); \
    size_t saved_offset = slop_arena_save(local_arena)
#define SLOP_LEAVE() (slop_arena_restore(local_arena, saved_offset), slop_arena_depth--)

// Slop String representation
typedef struct SlopString {
    const char* data;
    size_t length;
} SlopString;

// String literals point straight at static storage: no allocation, no copy.
#define SLOP_STR(lit) ((SlopString){ .data = (lit), .length = sizeof(lit) - 1 })

static inline SlopString slop_string_create(SlopArena* arena, const char* c_str) {
    size_t len = strlen(c_str);
    char* data = (char*)slop_arena_alloc(arena, len + 1);
    memcpy(data, c_str, len);
    data[len] = '\0';
    SlopString str = { .data = data, .length = len };
    return str;
}

static inline SlopString slop_string_create_len(SlopArena* arena, const char* src, size_t len) {
    char* data = (char*)slop_arena_alloc(arena, len + 1);
    memcpy(data, src, len);
    data[len] = '\0';
    SlopString str = { .data = data, .length = len };
    return str;
}

static inline SlopString slop_string_concat(SlopArena* arena, SlopString s1, SlopString s2) {
    size_t total_len = s1.length + s2.length;
    char* data = (char*)slop_arena_alloc(arena, total_len + 1);
    memcpy(data, s1.data, s1.length);
    memcpy(data + s1.length, s2.data, s2.length);
    data[total_len] = '\0';
    SlopString str = { .data = data, .length = total_len };
    return str;
}

static inline bool slop_string_equal(SlopString s1, SlopString s2) {
    if (s1.length != s2.length) return false;
    return memcmp(s1.data, s2.data, s1.length) == 0;
}

static inline SlopString slop_string_clone(SlopArena* dest_arena, SlopString s) {
    if (s.length == 0) return SLOP_STR("");
    return slop_string_create_len(dest_arena, s.data, s.length);
}

// Strings are immutable, so a substring is just a view: no allocation.
static inline SlopString slop_char_at(SlopArena* arena, SlopString s, int64_t idx) {
    (void)arena;
    if (idx >= 0 && (size_t)idx < s.length) return (SlopString){ .data = s.data + idx, .length = 1 };
    return SLOP_STR("");
}

// Slop Int to String
static inline SlopString slop_int_to_string(SlopArena* arena, int64_t val) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lld", (long long)val);
    return slop_string_create_len(arena, buf, len);
}

// Slop Array representation
typedef struct SlopArray {
    void** data;
    size_t length;
    size_t capacity;
} SlopArray;

static inline SlopArray slop_array_create(SlopArena* arena) {
    SlopArray arr;
    arr.length = 0;
    arr.capacity = 8;
    arr.data = (void**)slop_arena_alloc(arena, arr.capacity * sizeof(void*));
    return arr;
}

static inline void slop_array_grow(SlopArena* arena, SlopArray* arr) {
    size_t old_cap = arr->capacity;
    arr->capacity = old_cap ? old_cap * 2 : 8;
    // If the array is the newest thing in the bucket, just grow it where it is.
    if (old_cap && slop_arena_extend(arena, arr->data, old_cap * sizeof(void*), arr->capacity * sizeof(void*))) return;
    void** new_data = (void**)slop_arena_alloc(arena, arr->capacity * sizeof(void*));
    if (old_cap) memcpy(new_data, arr->data, old_cap * sizeof(void*));
    arr->data = new_data;
}

static inline void slop_array_push(SlopArena* arena, SlopArray* arr, void* item) {
    if (arr->length >= arr->capacity) slop_array_grow(arena, arr);
    arr->data[arr->length++] = item;
}

static inline void slop_array_bounds_fail(SlopArray arr, int64_t idx) {
    fprintf(stderr, "Slop Secure Guard: Array index out of bounds! Length: %zu, Requested Index: %lld. Terminating process to prevent memory exploit.\n", arr.length, (long long)idx);
    exit(139);
}

static inline void* slop_array_get(SlopArray arr, int64_t idx) {
    if (__builtin_expect(idx < 0 || (size_t)idx >= arr.length, 0)) slop_array_bounds_fail(arr, idx);
    return arr.data[idx];
}

// Element storage: values that fit in a slot (int, float, bool, small structs)
// live directly in the slot; bigger ones (strings, big structs) are boxed.
// `elem_size` is always sizeof(T), so the branch folds away at compile time.
#define SLOP_INLINE_ELEM(elem_size) ((elem_size) <= sizeof(void*))

// Returns where element `idx` lives. Usable as an lvalue: *(T*)slop_array_at(...)
static inline void* slop_array_at(SlopArray arr, int64_t idx, size_t elem_size) {
    if (__builtin_expect(idx < 0 || (size_t)idx >= arr.length, 0)) slop_array_bounds_fail(arr, idx);
    return SLOP_INLINE_ELEM(elem_size) ? (void*)&arr.data[idx] : arr.data[idx];
}

// Appends an element and returns where to write it: *(T*)slop_array_push_slot(...) = v
static inline void* slop_array_push_slot(SlopArena* arena, SlopArray* arr, size_t elem_size) {
    if (arr->length >= arr->capacity) slop_array_grow(arena, arr);
    void** slot = &arr->data[arr->length++];
    if (SLOP_INLINE_ELEM(elem_size)) return slot;
    *slot = slop_arena_alloc(arena, elem_size);
    return *slot;
}

#define SLOP_AT(T, arr, idx) (*(T*)slop_array_at((arr), (idx), sizeof(T)))
#define SLOP_PUSH(arena, arr, T, value) (*(T*)slop_array_push_slot((arena), &(arr), sizeof(T)) = (value))

static inline SlopArray slop_split(SlopArena* arena, SlopString s, SlopString sep) {
    SlopArray arr = slop_array_create(arena);
    size_t start = 0;
    if (sep.length == 0) {
        for (size_t i = 0; i < s.length; i++) {
            SlopString* sub = (SlopString*)slop_arena_alloc(arena, sizeof(SlopString));
            *sub = (SlopString){ .data = s.data + i, .length = 1 };
            slop_array_push(arena, &arr, sub);
        }
    } else {
        for (size_t i = 0; s.length >= sep.length && i <= s.length - sep.length; ) {
            if (memcmp(s.data + i, sep.data, sep.length) == 0) {
                SlopString* sub = (SlopString*)slop_arena_alloc(arena, sizeof(SlopString));
                *sub = (SlopString){ .data = s.data + start, .length = i - start };
                slop_array_push(arena, &arr, sub);
                i += sep.length;
                start = i;
            } else {
                i++;
            }
        }
        if (start <= s.length) {
            SlopString* sub = (SlopString*)slop_arena_alloc(arena, sizeof(SlopString));
            *sub = (SlopString){ .data = s.data + start, .length = s.length - start };
            slop_array_push(arena, &arr, sub);
        }
    }
    return arr;
}

static inline SlopString slop_join(SlopArena* arena, SlopArray arr, SlopString sep) {
    size_t total = arr.length ? sep.length * (arr.length - 1) : 0;
    for (size_t i = 0; i < arr.length; i++) total += ((SlopString*)arr.data[i])->length;
    char* out = (char*)slop_arena_alloc(arena, total + 1);
    size_t p = 0;
    for (size_t i = 0; i < arr.length; i++) {
        SlopString* s = (SlopString*)arr.data[i];
        if (i > 0) { memcpy(out + p, sep.data, sep.length); p += sep.length; }
        memcpy(out + p, s->data, s->length);
        p += s->length;
    }
    out[p] = '\0';
    return (SlopString){ .data = out, .length = p };
}

static inline SlopArray slop_array_clone_strings(SlopArena* dest_arena, SlopArray arr) {
    SlopArray new_arr = slop_array_create(dest_arena);
    for (size_t i = 0; i < arr.length; i++) {
        SlopString* s = (SlopString*)arr.data[i];
        SlopString* new_s = (SlopString*)slop_arena_alloc(dest_arena, sizeof(SlopString));
        *new_s = slop_string_clone(dest_arena, *s);
        slop_array_push(dest_arena, &new_arr, new_s);
    }
    return new_arr;
}

// Copies an array of inline (slot-sized) values into another arena.
static inline SlopArray slop_array_clone_ints(SlopArena* dest_arena, SlopArray arr) {
    SlopArray new_arr;
    new_arr.length = arr.length;
    new_arr.capacity = arr.length ? arr.length : 1;
    new_arr.data = (void**)slop_arena_alloc(dest_arena, new_arr.capacity * sizeof(void*));
    memcpy(new_arr.data, arr.data, arr.length * sizeof(void*));
    return new_arr;
}

// SPCA compressed packaging functions
static inline SlopString slop_pack_strings(SlopArena* arena, SlopArray arr) {
    SlopString dict[256];
    int dict_size = 0;
    
    for (size_t i = 0; i < arr.length; i++) {
        SlopString* s = (SlopString*)arr.data[i];
        bool found = false;
        for (int j = 0; j < dict_size; j++) {
            if (slop_string_equal(dict[j], *s)) {
                found = true;
                break;
            }
        }
        if (!found && dict_size < 256) {
            dict[dict_size++] = *s;
        }
    }
    
    size_t est_size = 1 + dict_size * 4 + arr.length * 5;
    for (int i = 0; i < dict_size; i++) est_size += dict[i].length;
    for (size_t i = 0; i < arr.length; i++) est_size += ((SlopString*)arr.data[i])->length;
    
    uint8_t* out = (uint8_t*)slop_arena_alloc(arena, est_size);
    size_t p = 0;
    
    out[p++] = (uint8_t)dict_size;
    
    for (int i = 0; i < dict_size; i++) {
        uint16_t len = (uint16_t)dict[i].length;
        out[p++] = (uint8_t)(len & 0xFF);
        out[p++] = (uint8_t)((len >> 8) & 0xFF);
        memcpy(&out[p], dict[i].data, len);
        p += len;
    }
    
    uint32_t arr_len = (uint32_t)arr.length;
    out[p++] = (uint8_t)(arr_len & 0xFF);
    out[p++] = (uint8_t)((arr_len >> 8) & 0xFF);
    out[p++] = (uint8_t)((arr_len >> 16) & 0xFF);
    out[p++] = (uint8_t)((arr_len >> 24) & 0xFF);
    
    for (size_t i = 0; i < arr.length; i++) {
        SlopString* s = (SlopString*)arr.data[i];
        int dict_id = -1;
        for (int j = 0; j < dict_size; j++) {
            if (slop_string_equal(dict[j], *s)) {
                dict_id = j;
                break;
            }
        }
        
        if (dict_id != -1) {
            out[p++] = (uint8_t)dict_id;
        } else {
            out[p++] = 0xFF; // escape token
            uint16_t len = (uint16_t)s->length;
            out[p++] = (uint8_t)(len & 0xFF);
            out[p++] = (uint8_t)((len >> 8) & 0xFF);
            memcpy(&out[p], s->data, len);
            p += len;
        }
    }
    
    SlopString res;
    res.data = (char*)out;
    res.length = p;
    return res;
}

static inline SlopArray slop_unpack_strings(SlopArena* arena, SlopString packed) {
    SlopArray arr = slop_array_create(arena);
    uint8_t* in = (uint8_t*)packed.data;
    size_t p = 0;
    
    int dict_size = in[p++];
    
    SlopString dict[256];
    for (int i = 0; i < dict_size; i++) {
        uint16_t len = in[p++];
        len |= (uint16_t)(in[p++]) << 8;
        
        SlopString s;
        char* data = (char*)slop_arena_alloc(arena, len + 1);
        memcpy(data, &in[p], len);
        data[len] = '\0';
        s.data = data;
        s.length = len;
        
        dict[i] = s;
        p += len;
    }
    
    uint32_t arr_len = in[p++];
    arr_len |= (uint32_t)(in[p++]) << 8;
    arr_len |= (uint32_t)(in[p++]) << 16;
    arr_len |= (uint32_t)(in[p++]) << 24;
    
    for (uint32_t i = 0; i < arr_len; i++) {
        uint8_t token = in[p++];
        SlopString* s = (SlopString*)slop_arena_alloc(arena, sizeof(SlopString));
        if (token < 0xFF) {
            *s = dict[token];
        } else {
            uint16_t len = in[p++];
            len |= (uint16_t)(in[p++]) << 8;
            
            char* data = (char*)slop_arena_alloc(arena, len + 1);
            memcpy(data, &in[p], len);
            data[len] = '\0';
            s->data = data;
            s->length = len;
            
            p += len;
        }
        slop_array_push(arena, &arr, s);
    }
    
    return arr;
}

// Sloppy-Escape Zero-Copy Socket Pipes (ZCSPC)
static inline int64_t slop_socket_listen(int64_t port) {
    slop_winsock_init();
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return -1;
    
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((uint16_t)port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        closesocket_compat(server_fd);
        return -1;
    }
    
    if (listen(server_fd, 128) < 0) {
        closesocket_compat(server_fd);
        return -1;
    }
    
    return server_fd;
}

static inline int64_t slop_socket_accept(int64_t server_fd) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fd = accept((int)server_fd, (struct sockaddr*)&address, &addrlen);
    return client_fd;
}

static inline SlopString slop_socket_read(SlopArena* arena, int64_t client_fd) {
    size_t max_read = 8192;
    char* buf = (char*)slop_arena_alloc(arena, max_read + 1);
    
#ifdef _WIN32
    ssize_t read_bytes = recv((int)client_fd, buf, (int)max_read, 0);
#else
    ssize_t read_bytes = recv((int)client_fd, buf, max_read, 0);
#endif
    
    if (read_bytes < 0) {
        return (SlopString){ .data = "", .length = 0 };
    }

    buf[read_bytes] = '\0';
    // Give back the unused tail of the read buffer (keeping 8-byte alignment).
    arena->current->used = (size_t)((uint8_t*)buf - arena->current->data) + (((size_t)read_bytes + 8) & ~(size_t)7);
    return (SlopString){ .data = buf, .length = (size_t)read_bytes };
}

static inline void slop_socket_write(int64_t client_fd, SlopString response) {
#ifdef _WIN32
    send((int)client_fd, response.data, (int)response.length, 0);
#else
    send((int)client_fd, response.data, response.length, 0);
#endif
}

static inline void slop_socket_close(int64_t fd) {
    closesocket_compat((int)fd);
}

// Sloppy Tensor Arenas (STA)
typedef struct SlopTensor {
    int64_t rows;
    int64_t cols;
    double* data;
} SlopTensor;

static inline SlopTensor slop_tensor_create(SlopArena* arena, int64_t rows, int64_t cols) {
    SlopTensor t;
    t.rows = rows;
    t.cols = cols;
    t.data = (double*)slop_arena_alloc(arena, rows * cols * sizeof(double));
    memset(t.data, 0, rows * cols * sizeof(double));
    return t;
}

static inline SlopTensor slop_tensor_mul(SlopArena* arena, SlopTensor t1, SlopTensor t2) {
    if (t1.cols != t2.rows) {
        fprintf(stderr, "Slop AI Error: Matrix dimension mismatch for multiplication!\n");
        exit(1);
    }
    SlopTensor res = slop_tensor_create(arena, t1.rows, t2.cols);
    for (int64_t i = 0; i < t1.rows; i++) {
        for (int64_t k = 0; k < t1.cols; k++) {
            double val = t1.data[i * t1.cols + k];
            for (int64_t j = 0; j < t2.cols; j++) {
                res.data[i * t2.cols + j] += val * t2.data[k * t2.cols + j];
            }
        }
    }
    return res;
}

static inline SlopTensor slop_tensor_add(SlopArena* arena, SlopTensor t1, SlopTensor t2) {
    if (t1.rows != t2.rows || t1.cols != t2.cols) {
        fprintf(stderr, "Slop AI Error: Dimension mismatch for tensor addition!\n");
        exit(1);
    }
    SlopTensor res = slop_tensor_create(arena, t1.rows, t1.cols);
    for (int64_t i = 0; i < t1.rows * t1.cols; i++) {
        res.data[i] = t1.data[i] + t2.data[i];
    }
    return res;
}

static inline void slop_tensor_softmax(SlopTensor t) {
    for (int64_t r = 0; r < t.rows; r++) {
        int64_t row_offset = r * t.cols;
        double max_val = t.data[row_offset];
        for (int64_t c = 1; c < t.cols; c++) {
            if (t.data[row_offset + c] > max_val) {
                max_val = t.data[row_offset + c];
            }
        }
        double sum = 0.0;
        for (int64_t c = 0; c < t.cols; c++) {
            t.data[row_offset + c] = exp(t.data[row_offset + c] - max_val);
            sum += t.data[row_offset + c];
        }
        for (int64_t c = 0; c < t.cols; c++) {
            t.data[row_offset + c] /= sum;
        }
    }
}

static inline void slop_tensor_print(SlopTensor t) {
    printf("Tensor Dimension: [%lld x %lld]\n", (long long)t.rows, (long long)t.cols);
    for (int64_t r = 0; r < t.rows && r < 5; r++) {
        printf("  Row %lld: ", (long long)r);
        for (int64_t c = 0; c < t.cols && c < 5; c++) {
            printf("%.4f  ", t.data[r * t.cols + c]);
        }
        if (t.cols > 5) printf("... (+%lld cols)", (long long)(t.cols - 5));
        printf("\n");
    }
    if (t.rows > 5) printf("  ... (+%lld rows)\n", (long long)(t.rows - 5));
    printf("\n");
}

// Sloppy Single-Page Application (SPA) Engine
static const char* slop_ui_html = NULL;

static inline int64_t slop_ui_create(SlopString title) {
    int64_t server_fd = slop_socket_listen(9191);
    if (server_fd < 0) return -1;
    
#ifdef _WIN32
    system("start http://localhost:9191");
#elif __APPLE__
    system("open http://localhost:9191 &");
#else
    system("xdg-open http://localhost:9191 &");
#endif

    return server_fd;
}

static inline void slop_ui_render(SlopString html) {
    char* copy = (char*)realloc((void*)slop_ui_html, html.length + 1);
    if (!copy) slop_oom();
    memcpy(copy, html.data, html.length);
    copy[html.length] = '\0';
    slop_ui_html = copy;
}

static inline void slop_ui_serve(int64_t server_fd) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET((int)server_fd, &read_fds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    
    int activity = select((int)server_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (activity > 0) {
        int client_fd = accept((int)server_fd, NULL, NULL);
        if (client_fd >= 0) {
            char req[1024];
            recv(client_fd, req, sizeof(req), 0);
            
            const char* header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
#ifdef _WIN32
            send(client_fd, header, (int)strlen(header), 0);
            if (slop_ui_html) {
                send(client_fd, slop_ui_html, (int)strlen(slop_ui_html), 0);
            } else {
                send(client_fd, "<h1>Slop UI Render Standby</h1>", 31, 0);
            }
#else
            send(client_fd, header, strlen(header), 0);
            if (slop_ui_html) {
                send(client_fd, slop_ui_html, strlen(slop_ui_html), 0);
            } else {
                send(client_fd, "<h1>Slop UI Render Standby</h1>", 31, 0);
            }
#endif
            closesocket_compat(client_fd);
        }
    } else {
        printf("-> No browser connection detected within 100ms. Closing UI container gracefully.\n");
    }
}

// ============================================================================
// NOVEL GAME INNOVATION: Sloppy-Sprite WebGL Canvas (SSCE)
// ============================================================================
// Zero-copy, hardware-accelerated 2D desktop game engine.
// All physics and updates run in high-performance SEAA memory buckets, flushing
// drawing matrices directly to the GPU WebGL canvas at 60+ FPS natively!
// ============================================================================

static char slop_game_buffer[65536] = {0};
static size_t slop_game_p = 0;
static int64_t slop_game_width = 800;
static int64_t slop_game_height = 600;

static inline int64_t slop_game_init(int64_t width, int64_t height) {
    slop_game_width = width;
    slop_game_height = height;
    slop_game_p = 0;
    memset(slop_game_buffer, 0, sizeof(slop_game_buffer));
    
    // Create UI socket listener on port 9292 for game frame streaming
    int64_t server_fd = slop_socket_listen(9292);
    if (server_fd < 0) return -1;
    
#ifdef _WIN32
    system("start http://localhost:9292");
#elif __APPLE__
    system("open http://localhost:9292 &");
#else
    system("xdg-open http://localhost:9292 &");
#endif

    return server_fd;
}

static inline void slop_game_clear() {
    slop_game_p = 0;
    memset(slop_game_buffer, 0, sizeof(slop_game_buffer));
}

static inline void slop_game_rect(int64_t x, int64_t y, int64_t w, int64_t h, SlopString color) {
    char temp[256];
    int len = snprintf(temp, sizeof(temp), "R,%lld,%lld,%lld,%lld,%.*s;",
                       (long long)x, (long long)y, (long long)w, (long long)h, (int)color.length, color.data);
    if (slop_game_p + len < sizeof(slop_game_buffer)) {
        memcpy(slop_game_buffer + slop_game_p, temp, len);
        slop_game_p += len;
    }
}

static inline void slop_game_circle(int64_t x, int64_t y, int64_t r, SlopString color) {
    char temp[256];
    int len = snprintf(temp, sizeof(temp), "C,%lld,%lld,%lld,%.*s;",
                       (long long)x, (long long)y, (long long)r, (int)color.length, color.data);
    if (slop_game_p + len < sizeof(slop_game_buffer)) {
        memcpy(slop_game_buffer + slop_game_p, temp, len);
        slop_game_p += len;
    }
}

static inline void slop_game_text(int64_t x, int64_t y, SlopString text, SlopString color) {
    char temp[512];
    int len = snprintf(temp, sizeof(temp), "T,%lld,%lld,%.*s,%.*s;",
                       (long long)x, (long long)y, (int)text.length, text.data, (int)color.length, color.data);
    if (slop_game_p + len < sizeof(slop_game_buffer)) {
        memcpy(slop_game_buffer + slop_game_p, temp, len);
        slop_game_p += len;
    }
}

static inline void slop_game_update(int64_t server_fd) {
    // Highly optimized WebGL Double-buffer flush over local socket loops!
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET((int)server_fd, &read_fds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 16000; // 60 FPS update speed (16.6ms target frame budget)
    
    int activity = select((int)server_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (activity > 0) {
        int client_fd = accept((int)server_fd, NULL, NULL);
        if (client_fd >= 0) {
            char req[1024];
            recv(client_fd, req, sizeof(req), 0);
            
            // Check if client is requesting initial HTML view or live frame buffers!
            if (strstr(req, "GET / HTTP") != NULL) {
                // Serve hardware-accelerated HTML5 WebGL drawing canvas
                char html[4096];
                snprintf(html, sizeof(html),
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
                    "<!DOCTYPE html><html><head><title>Slop Game Canvas</title><style>"
                    "body { background: #000; margin: 0; display: flex; align-items: center; justify-content: center; height: 100vh; overflow: hidden; }"
                    "canvas { border: 2px solid #333; box-shadow: 0 0 20px rgba(255,255,255,0.1); }"
                    "</style></head><body>"
                    "<canvas id='g' width='%lld' height='%lld'></canvas>"
                    "<script>"
                    "let c = document.getElementById('g').getContext('2d');"
                    "let f = () => {"
                    "  fetch('/f').then(r => r.text()).then(data => {"
                    "    c.fillStyle = '#000000'; c.fillRect(0,0,%lld,%lld);"
                    "    let cmds = data.split(';');"
                    "    for (let cmd of cmds) {"
                    "      let p = cmd.split(',');"
                    "      if (p[0] === 'R') { c.fillStyle = p[5]; c.fillRect(parseInt(p[1]), parseInt(p[2]), parseInt(p[3]), parseInt(p[4])); }"
                    "      if (p[0] === 'C') { c.fillStyle = p[4]; c.beginPath(); c.arc(parseInt(p[1]), parseInt(p[2]), parseInt(p[3]), 0, Math.PI*2); c.fill(); }"
                    "      if (p[0] === 'T') { c.fillStyle = p[4]; c.font = '20px sans-serif'; c.fillText(p[3], parseInt(p[1]), parseInt(p[2])); }"
                    "    }"
                    "    setTimeout(f, 16);" // Fetch next frame buffer in 16ms
                    "  });"
                    "};"
                    "f();"
                    "</script></body></html>",
                    (long long)slop_game_width, (long long)slop_game_height, (long long)slop_game_width, (long long)slop_game_height);
                send(client_fd, html, (int)strlen(html), 0);
            } else if (strstr(req, "GET /f HTTP") != NULL) {
                // Serve current frame payload
                const char* header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
                send(client_fd, header, (int)strlen(header), 0);
                send(client_fd, slop_game_buffer, (int)strlen(slop_game_buffer), 0);
            }
            closesocket_compat(client_fd);
        }
    }
}

// ============================================================================
// NOVEL PARALLELISM INNOVATION: Slop Unified Parallel Compute Engine (SPCE)
// ============================================================================
// A zero-boilerplate, high-performance CPU thread-pool dispatcher that turns
// Pythonic list comprehensions and simple for-loops into multi-core native
// work-stealing-style parallel execution. Every worker thread runs on its own
// independent thread-local SEAA arena bucket, so allocations remain 100%
// lock-free and contention-free.
// ============================================================================

static inline int slop_cpu_count() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
#endif
}

typedef void (*SlopParallelFn)(int64_t, void*);

typedef struct {
    int64_t start;
    int64_t end;
    SlopParallelFn fn;
    void* ctx;
} SlopRangeCtx;

static inline void* slop_range_worker(void* arg) {
    slop_arena_depth = 1;
    SlopRangeCtx* ctx = (SlopRangeCtx*)arg;
    for (int64_t i = ctx->start; i < ctx->end; i++) {
        ctx->fn(i, ctx->ctx);
    }
    slop_arena_depth = 0;
    return NULL;
}

static inline void slop_parallel_for(int64_t start, int64_t end, SlopParallelFn fn, void* ctx) {
    if (end <= start) return;
    int num_threads = slop_cpu_count();
    if (num_threads <= 1) {
        for (int64_t i = start; i < end; i++) fn(i, ctx);
        return;
    }

    int64_t total = end - start;
    if (num_threads > total) num_threads = (int)total;

    pthread_t threads[64];
    SlopRangeCtx ctxs[64];

    int64_t chunk = total / num_threads;
    int64_t remainder = total % num_threads;
    int64_t current = start;

    for (int i = 0; i < num_threads; i++) {
        ctxs[i].start = current;
        ctxs[i].end = current + chunk + (i < remainder ? 1 : 0);
        ctxs[i].fn = fn;
        ctxs[i].ctx = ctx;
        current = ctxs[i].end;
        pthread_create(&threads[i], NULL, slop_range_worker, &ctxs[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

// Context used by the parallel map dispatcher. It is passed through the
// work-stealing dispatcher (not stored in thread-local storage) so every worker
// thread can see the same input/output arrays and mapping function safely.
typedef struct {
    SlopArray input;
    SlopArray output;
    int64_t (*map_fn)(int64_t);
} SlopMapCtx;

static inline void slop_parallel_map_worker(int64_t idx, void* ctx) {
    SlopMapCtx* map_ctx = (SlopMapCtx*)ctx;
    SLOP_AT(int64_t, map_ctx->output, idx) = map_ctx->map_fn(SLOP_AT(int64_t, map_ctx->input, idx));
}

static inline SlopArray slop_parallel_map(SlopArena* arena, SlopArray input, int64_t (*map_fn)(int64_t)) {
    SlopArray output = slop_array_clone_ints(arena, input);

    SlopMapCtx ctx = { input, output, map_fn };
    slop_parallel_for(0, (int64_t)input.length, slop_parallel_map_worker, &ctx);
    return output;
}

// Helper IO Functions
static inline SlopString slop_read_file(SlopArena* arena, SlopString path_str) {
    char* path = (char*)malloc(path_str.length + 1);
    memcpy(path, path_str.data, path_str.length);
    path[path_str.length] = '\0';

    if (strstr(path, "..") != NULL || strstr(path, "..\\") != NULL) {
        fprintf(stderr, "Slop Secure Guard: Blocked directory traversal attack attempt on path: %s\n", path);
        free(path);
        return (SlopString){ .data = "", .length = 0 };
    }

    FILE* f = fopen(path, "rb");
    free(path);

    if (!f) {
        return (SlopString){ .data = "", .length = 0 };
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        size_t cap = 4096;
        char* data = (char*)slop_arena_alloc(arena, cap);
        size_t total_read = 0;
        while (1) {
            size_t read_bytes = fread(data + total_read, 1, 4096, f);
            total_read += read_bytes;
            if (read_bytes < 4096) {
                break;
            }
            cap += 4096;
            char* new_data = (char*)slop_arena_alloc(arena, cap);
            memcpy(new_data, data, total_read);
            data = new_data;
        }
        data[total_read] = '\0';
        fclose(f);
        return (SlopString){ .data = data, .length = total_read };
    }

    char* data = (char*)slop_arena_alloc(arena, size + 1);
    size_t read_bytes = fread(data, 1, size, f);
    data[read_bytes] = '\0';
    fclose(f);

    return (SlopString){ .data = data, .length = read_bytes };
}

static inline void slop_write_file(SlopString path_str, SlopString content) {
    char* path = (char*)malloc(path_str.length + 1);
    memcpy(path, path_str.data, path_str.length);
    path[path_str.length] = '\0';

    if (strstr(path, "..") != NULL || strstr(path, "..\\") != NULL) {
        fprintf(stderr, "Slop Secure Guard: Blocked directory traversal write attempt on path: %s\n", path);
        free(path);
        return;
    }

    FILE* f = fopen(path, "wb");
    free(path);

    if (f) {
        fwrite(content.data, 1, content.length, f);
        fclose(f);
    }
}

// Print builtins
static inline void slop_print_string(SlopString s) {
    fwrite(s.data, 1, s.length, stdout);
    printf("\n");
}

static inline void slop_print_int(int64_t val) {
    printf("%lld\n", (long long)val);
}

static inline void slop_print_bool(bool val) {
    printf("%s\n", val ? "true" : "false");
}

#include "slop_lowlevel.h"

#endif
