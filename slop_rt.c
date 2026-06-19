#define SLOP_IMPLEMENTATION
#include "slop_rt.h"

int slop_arena_depth = 0;

// Export linkable versions of the runtime functions for Rust and other FFI users
SlopArena* ext_slop_arena_create(size_t capacity) {
    return slop_arena_create(capacity);
}

void ext_slop_arena_destroy(SlopArena* arena) {
    slop_arena_destroy(arena);
}

void* ext_slop_arena_alloc(SlopArena* arena, size_t size) {
    return slop_arena_alloc(arena, size);
}

size_t ext_slop_arena_save(SlopArena* arena) {
    return slop_arena_save(arena);
}

void ext_slop_arena_restore(SlopArena* arena, size_t offset) {
    slop_arena_restore(arena, offset);
}

SlopArena* ext_slop_get_arena(int depth) {
    return slop_get_arena(depth);
}

SlopString ext_slop_string_create_len(SlopArena* arena, const char* src, size_t len) {
    return slop_string_create_len(arena, src, len);
}

SlopString ext_slop_string_concat(SlopArena* arena, SlopString s1, SlopString s2) {
    return slop_string_concat(arena, s1, s2);
}

bool ext_slop_string_equal(SlopString s1, SlopString s2) {
    return slop_string_equal(s1, s2);
}

SlopArray ext_slop_array_create(SlopArena* arena) {
    return slop_array_create(arena);
}

void ext_slop_array_push(SlopArena* arena, SlopArray* arr, void* item) {
    slop_array_push(arena, arr, item);
}

void ext_slop_print_string(SlopString s) {
    slop_print_string(s);
}

void ext_slop_print_int(int64_t val) {
    slop_print_int(val);
}
