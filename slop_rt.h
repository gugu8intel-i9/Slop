#ifndef SLOP_RT_H
#define SLOP_RT_H

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Slop Arena / Bucket Structure
typedef struct SlopArena {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
} SlopArena;

static inline SlopArena* slop_arena_create(size_t capacity) {
    SlopArena* arena = (SlopArena*)malloc(sizeof(SlopArena));
    if (!arena) {
        fprintf(stderr, "Out of memory allocating Arena metadata.\n");
        exit(1);
    }
    arena->buffer = (uint8_t*)malloc(capacity);
    if (!arena->buffer) {
        fprintf(stderr, "Out of memory allocating Arena buffer of size %zu.\n", capacity);
        exit(1);
    }
    // Securely initialize buffer to prevent uninitialized memory reads
    memset(arena->buffer, 0, capacity);
    arena->capacity = capacity;
    arena->offset = 0;
    return arena;
}

static inline void slop_arena_destroy(SlopArena* arena) {
    if (arena) {
        // Securely scrub memory before freeing to prevent RAM dumps
        memset(arena->buffer, 0, arena->capacity);
        free(arena->buffer);
        free(arena);
    }
}

static inline void* slop_arena_alloc(SlopArena* arena, size_t size) {
    size = (size + 7) & ~7;
    if (arena->offset + size > arena->capacity) {
        size_t new_capacity = arena->capacity * 2;
        if (new_capacity < arena->offset + size) {
            new_capacity = arena->offset + size + 1024 * 1024;
        }
        uint8_t* new_buf = (uint8_t*)realloc(arena->buffer, new_capacity);
        if (!new_buf) {
            fprintf(stderr, "Slop Bucket Overflow and resize failed! Required: %zu bytes.\n", arena->offset + size);
            exit(1);
        }
        // Initialize newly allocated capacity block to 0
        memset(&new_buf[arena->capacity], 0, new_capacity - arena->capacity);
        arena->buffer = new_buf;
        arena->capacity = new_capacity;
    }
    void* ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    return ptr;
}

static inline size_t slop_arena_save(SlopArena* arena) {
    return arena->offset;
}

// SECURE MEMORY SANITIZATION: Zero out discarded memory upon scope exit
static inline void slop_arena_restore(SlopArena* arena, size_t offset) {
    if (arena->offset > offset) {
        // Securely zero out (scrub) all memory between the current offset and the new offset
        // This ensures no password, key, or token leftovers persist in physical RAM.
        memset(&arena->buffer[offset], 0, arena->offset - offset);
    }
    arena->offset = offset;
}

// Global Arena Stack
extern int slop_arena_depth;
static inline SlopArena* slop_get_arena(int depth) {
    static SlopArena* arenas[1024] = {NULL};
    if (depth < 0 || depth >= 1024) {
        fprintf(stderr, "Slop call depth limit exceeded! Max: 1024.\n");
        exit(1);
    }
    if (!arenas[depth]) {
        arenas[depth] = slop_arena_create(64 * 1024); // 64KB dynamic starting capacity
    }
    return arenas[depth];
}

// Slop String representation
typedef struct SlopString {
    const char* data;
    size_t length;
} SlopString;

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
    if (s.length == 0) return (SlopString){ .data = "", .length = 0 };
    return slop_string_create_len(dest_arena, s.data, s.length);
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

static inline void slop_array_push(SlopArena* arena, SlopArray* arr, void* item) {
    if (arr->length >= arr->capacity) {
        size_t old_cap = arr->capacity;
        arr->capacity *= 2;
        void** new_data = (void**)slop_arena_alloc(arena, arr->capacity * sizeof(void*));
        memcpy(new_data, arr->data, old_cap * sizeof(void*));
        arr->data = new_data;
    }
    arr->data[arr->length++] = item;
}

// SECURE BOUNDS CHECKING: Prevents buffer overflows and arbitrary memory reading exploits
static inline void* slop_array_get(SlopArray arr, int64_t idx) {
    if (idx < 0 || (size_t)idx >= arr.length) {
        fprintf(stderr, "Slop Secure Guard: Array index out of bounds! Length: %zu, Requested Index: %lld. Terminating process to prevent memory exploit.\n", arr.length, (long long)idx);
        exit(139); // Terminate safely with segmentation fault exit code
    }
    return arr.data[idx];
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

static inline SlopArray slop_array_clone_ints(SlopArena* dest_arena, SlopArray arr) {
    SlopArray new_arr = slop_array_create(dest_arena);
    for (size_t i = 0; i < arr.length; i++) {
        int64_t* val = (int64_t*)arr.data[i];
        int64_t* new_val = (int64_t*)slop_arena_alloc(dest_arena, sizeof(int64_t));
        *new_val = *val;
        slop_array_push(dest_arena, &new_arr, new_val);
    }
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

// Helper IO Functions
static inline SlopString slop_read_file(SlopArena* arena, SlopString path_str) {
    char* path = (char*)malloc(path_str.length + 1);
    memcpy(path, path_str.data, path_str.length);
    path[path_str.length] = '\0';

    // SECURE DIRECTORY TRAVERSAL PROTECTION: Block "../" attacks
    if (strstr(path, "..") != NULL) {
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

    // SECURE DIRECTORY TRAVERSAL PROTECTION: Block "../" attacks
    if (strstr(path, "..") != NULL) {
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

#endif
