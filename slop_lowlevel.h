#ifndef SLOP_LOWLEVEL_H
#define SLOP_LOWLEVEL_H

// Optional unsafe low-level control layer for Slop.
// ------------------------------------------------
// This is deliberately NOT the normal path. Normal Slop remains safe and easy.
// These functions are for kernels, drivers, embedded-style experiments, hardware
// labs, allocators, GPU runtimes, and performance instrumentation.
//
// All APIs are prefixed with unsafe_/mmio_/cpu_/gpu_/ram_ to make the danger
// obvious at the call site.

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdatomic.h>

#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif

static inline uint8_t slop_unsafe_load8(uintptr_t addr) { return *(volatile uint8_t*)addr; }
static inline uint16_t slop_unsafe_load16(uintptr_t addr) { return *(volatile uint16_t*)addr; }
static inline uint32_t slop_unsafe_load32(uintptr_t addr) { return *(volatile uint32_t*)addr; }
static inline uint64_t slop_unsafe_load64(uintptr_t addr) { return *(volatile uint64_t*)addr; }

static inline void slop_unsafe_store8(uintptr_t addr, uint8_t value) { *(volatile uint8_t*)addr = value; }
static inline void slop_unsafe_store16(uintptr_t addr, uint16_t value) { *(volatile uint16_t*)addr = value; }
static inline void slop_unsafe_store32(uintptr_t addr, uint32_t value) { *(volatile uint32_t*)addr = value; }
static inline void slop_unsafe_store64(uintptr_t addr, uint64_t value) { *(volatile uint64_t*)addr = value; }

static inline uint32_t slop_mmio_read32(uintptr_t addr) {
    atomic_thread_fence(memory_order_seq_cst);
    uint32_t v = *(volatile uint32_t*)addr;
    atomic_thread_fence(memory_order_seq_cst);
    return v;
}

static inline void slop_mmio_write32(uintptr_t addr, uint32_t value) {
    atomic_thread_fence(memory_order_seq_cst);
    *(volatile uint32_t*)addr = value;
    atomic_thread_fence(memory_order_seq_cst);
}

static inline void slop_cpu_fence(void) { atomic_thread_fence(memory_order_seq_cst); }
static inline void slop_device_fence(void) { atomic_thread_fence(memory_order_seq_cst); }
static inline void slop_gpu_fence(void) { atomic_thread_fence(memory_order_seq_cst); }

static inline void slop_cpu_relax(void) {
#if defined(__i386__) || defined(__x86_64__)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield" ::: "memory");
#elif defined(__riscv)
    __asm__ __volatile__("pause" ::: "memory");
#else
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

static inline void slop_cpu_prefetch(uintptr_t addr) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch((const void*)addr, 0, 3);
#else
    (void)addr;
#endif
}

static inline uint64_t slop_cpu_cycles(void) {
#if defined(__i386__) || defined(__x86_64__)
    return (uint64_t)__rdtsc();
#elif defined(__aarch64__)
    uint64_t v = 0;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#elif defined(__riscv)
    uint64_t v = 0;
    __asm__ __volatile__("rdcycle %0" : "=r"(v));
    return v;
#else
    return 0;
#endif
}

static inline void slop_ram_copy(uintptr_t dst, uintptr_t src, uint64_t n) {
    memcpy((void*)dst, (const void*)src, (size_t)n);
}

static inline void slop_ram_zero(uintptr_t dst, uint64_t n) {
    memset((void*)dst, 0, (size_t)n);
}

static inline uintptr_t slop_component_base(uintptr_t addr) { return addr; }

#endif // SLOP_LOWLEVEL_H
