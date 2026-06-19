#ifndef SLOP_BRIDGE_HPP
#define SLOP_BRIDGE_HPP

#include "slop_rt.h"
#include <string>
#include <vector>
#include <string_view>
#include <functional>
#include <iostream>

// The Slop C++ High-Performance Native Bridge
namespace slop {

    // RAII Wrapper for the Slop Call Stack / Arena lifecycle
    class Scope {
    public:
        Scope() {
            slop_arena_depth++;
            // Retrieve or initialize the arena for this call-depth
            arena_ = slop_get_arena(slop_arena_depth);
            saved_offset_ = slop_arena_save(arena_);
        }

        ~Scope() {
            // Instant O(1) memory reclamation of all temporary allocations within this scope
            slop_arena_restore(arena_, saved_offset_);
            slop_arena_depth--;
        }

        SlopArena* arena() const { return arena_; }

    private:
        SlopArena* arena_;
        size_t saved_offset_;
    };

    // Zero-copy bridge for Strings
    inline SlopString to_slop_string(SlopArena* arena, std::string_view cpp_str) {
        // Creates a SlopString pointing to C++ string data (zero-copy)
        // Note: The C++ string must outlive the SlopString's usage in the arena
        SlopString s;
        s.data = cpp_str.data();
        s.length = cpp_str.length();
        return s;
    }

    inline SlopString copy_to_slop_string(SlopArena* arena, std::string_view cpp_str) {
        // Copy-based bridge: allocates memory inside the Slop Bucket arena
        return slop_string_create_len(arena, cpp_str.data(), cpp_str.length());
    }

    inline std::string_view to_cpp_string(SlopString slop_str) {
        return std::string_view(slop_str.data, slop_str.length);
    }

    // Zero-copy bridge for Arrays
    template<typename T>
    inline SlopArray to_slop_array(SlopArena* arena, const std::vector<T>& cpp_vec) {
        SlopArray arr = slop_array_create(arena);
        for (const auto& item : cpp_vec) {
            T* ptr = (T*)slop_arena_alloc(arena, sizeof(T));
            *ptr = item;
            slop_array_push(arena, &arr, ptr);
        }
        return arr;
    }

    template<typename T>
    inline std::vector<T> to_cpp_vector(SlopArray slop_arr) {
        std::vector<T> vec;
        vec.reserve(slop_arr.length);
        for (size_t i = 0; i < slop_arr.length; ++i) {
            vec.push_back(*(T*)slop_arr.data[i]);
        }
        return vec;
    }

    // High-performance Bridge execution wrapper
    inline void run_native(const std::function<void(SlopArena*)>& func) {
        Scope scope;
        func(scope.arena());
    }

} // namespace slop

#endif // SLOP_BRIDGE_HPP
