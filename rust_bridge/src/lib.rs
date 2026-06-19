use std::ffi::{c_void};
use std::os::raw::c_char;

// Raw FFI representations of Slop's core structures
#[repr(C)]
pub struct SlopArena {
    pub buffer: *mut u8,
    pub capacity: usize,
    pub offset: usize,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct SlopString {
    pub data: *const c_char,
    pub length: usize,
}

#[repr(C)]
pub struct SlopArray {
    pub data: *mut *mut c_void,
    pub length: usize,
    pub capacity: usize,
}

// Global state and FFI bindings linking to slop_rt.c
extern "C" {
    pub static mut slop_arena_depth: i32;
    
    #[link_name = "ext_slop_arena_create"]
    pub fn slop_arena_create(capacity: usize) -> *mut SlopArena;
    
    #[link_name = "ext_slop_arena_destroy"]
    pub fn slop_arena_destroy(arena: *mut SlopArena);
    
    #[link_name = "ext_slop_arena_alloc"]
    pub fn slop_arena_alloc(arena: *mut SlopArena, size: usize) -> *mut c_void;
    
    #[link_name = "ext_slop_arena_save"]
    pub fn slop_arena_save(arena: *mut SlopArena) -> usize;
    
    #[link_name = "ext_slop_arena_restore"]
    pub fn slop_arena_restore(arena: *mut SlopArena, offset: usize);
    
    #[link_name = "ext_slop_get_arena"]
    pub fn slop_get_arena(depth: i32) -> *mut SlopArena;
    
    #[link_name = "ext_slop_string_create_len"]
    pub fn slop_string_create_len(arena: *mut SlopArena, src: *const c_char, len: usize) -> SlopString;
    
    #[link_name = "ext_slop_string_concat"]
    pub fn slop_string_concat(arena: *mut SlopArena, s1: SlopString, s2: SlopString) -> SlopString;
    
    #[link_name = "ext_slop_string_equal"]
    pub fn slop_string_equal(s1: SlopString, s2: SlopString) -> bool;
    
    #[link_name = "ext_slop_array_create"]
    pub fn slop_array_create(arena: *mut SlopArena) -> SlopArray;
    
    #[link_name = "ext_slop_array_push"]
    pub fn slop_array_push(arena: *mut SlopArena, arr: *mut SlopArray, item: *mut c_void);
    
    #[link_name = "ext_slop_print_string"]
    pub fn slop_print_string(s: SlopString);
    
    #[link_name = "ext_slop_print_int"]
    pub fn slop_print_int(val: i64);
}

// Safe Rust Wrapper implementing the RAII call stack scope
pub struct SlopScope {
    arena_ptr: *mut SlopArena,
    saved_offset: usize,
}

impl SlopScope {
    pub fn new() -> Self {
        unsafe {
            slop_arena_depth += 1;
            let arena_ptr = slop_get_arena(slop_arena_depth);
            let saved_offset = slop_arena_save(arena_ptr);
            SlopScope {
                arena_ptr,
                saved_offset,
            }
        }
    }

    pub fn arena(&self) -> *mut SlopArena {
        self.arena_ptr
    }

    // Allocate a safe Rust string into the Slop arena (zero-copy)
    pub fn create_string(&self, s: &str) -> SlopString {
        unsafe {
            slop_string_create_len(self.arena_ptr, s.as_ptr() as *const c_char, s.len())
        }
    }

    // Convert a SlopString back to a standard Rust string slice
    pub fn to_rust_str(&self, s: SlopString) -> &str {
        unsafe {
            let bytes = std::slice::from_raw_parts(s.data as *const u8, s.length);
            std::str::from_utf8_unchecked(bytes)
        }
    }

    // Convert a Rust vector of i64 into a high-performance SlopArray
    pub fn create_int_array(&self, vec: &[i64]) -> SlopArray {
        unsafe {
            let mut arr = slop_array_create(self.arena_ptr);
            for &item in vec {
                let ptr = slop_arena_alloc(self.arena_ptr, std::mem::size_of::<i64>()) as *mut i64;
                *ptr = item;
                slop_array_push(self.arena_ptr, &mut arr, ptr as *mut c_void);
            }
            arr
        }
    }
}

impl Drop for SlopScope {
    fn drop(&mut self) {
        unsafe {
            // Instant O(1) reclamation of all memory allocated in this scope
            slop_arena_restore(self.arena_ptr, self.saved_offset);
            slop_arena_depth -= 1;
        }
    }
}

// High-performance Bridge run helper
pub fn run_native<F>(f: F)
where
    F: FnOnce(&SlopScope),
{
    let scope = SlopScope::new();
    f(&scope);
}
