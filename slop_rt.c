#include "slop_rt.h"

THREAD_LOCAL int slop_arena_depth = 0;

// Export all runtime functions with standard external linkage for FFI
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

// SPCA compressed packaging functions exported for FFI
SlopString ext_slop_pack_strings(SlopArena* arena, SlopArray arr) {
    return slop_pack_strings(arena, arr);
}

SlopArray ext_slop_unpack_strings(SlopArena* arena, SlopString packed) {
    return slop_unpack_strings(arena, packed);
}

// Zero-Copy Streaming Packet Channel Sockets exported for FFI
int64_t ext_slop_socket_listen(int64_t port) {
    return slop_socket_listen(port);
}

int64_t ext_slop_socket_accept(int64_t server_fd) {
    return slop_socket_accept(server_fd);
}

SlopString ext_slop_socket_read(SlopArena* arena, int64_t client_fd) {
    return slop_socket_read(arena, client_fd);
}

void ext_slop_socket_write(int64_t client_fd, SlopString content) {
    slop_socket_write(client_fd, content);
}

void ext_slop_socket_close(int64_t fd) {
    slop_socket_close(fd);
}

// Sloppy Tensor Arena (STA) exports for FFI
SlopTensor ext_slop_tensor_create(SlopArena* arena, int64_t rows, int64_t cols) {
    return slop_tensor_create(arena, rows, cols);
}

SlopTensor ext_slop_tensor_mul(SlopArena* arena, SlopTensor t1, SlopTensor t2) {
    return slop_tensor_mul(arena, t1, t2);
}

SlopTensor ext_slop_tensor_add(SlopArena* arena, SlopTensor t1, SlopTensor t2) {
    return slop_tensor_add(arena, t1, t2);
}

void ext_slop_tensor_softmax(SlopTensor t) {
    slop_tensor_softmax(t);
}

void ext_slop_tensor_print(SlopTensor t) {
    slop_tensor_print(t);
}

// Novel UI/UX exports for FFI
int64_t ext_slop_ui_create(SlopString title) {
    return slop_ui_create(title);
}

void ext_slop_ui_render(SlopString html) {
    slop_ui_render(html);
}

void ext_slop_ui_serve(int64_t server_fd) {
    slop_ui_serve(server_fd);
}

// Sloppy-Sprite WebGL Canvas (SSCE) Game Engine exports for FFI
int64_t ext_slop_game_init(int64_t width, int64_t height) {
    return slop_game_init(width, height);
}

void ext_slop_game_clear() {
    slop_game_clear();
}

void ext_slop_game_rect(int64_t x, int64_t y, int64_t w, int64_t h, SlopString color) {
    slop_game_rect(x, y, w, h, color);
}

void ext_slop_game_circle(int64_t x, int64_t y, int64_t r, SlopString color) {
    slop_game_circle(x, y, r, color);
}

void ext_slop_game_text(int64_t x, int64_t y, SlopString text, SlopString color) {
    slop_game_text(x, y, text, color);
}

void ext_slop_game_update(int64_t server_fd) {
    slop_game_update(server_fd);
}
