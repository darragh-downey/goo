#ifndef GOO_ZIG_RUNTIME_H
#define GOO_ZIG_RUNTIME_H

/**
 * Zig Runtime Integration for Goo
 * This header defines the interface between Goo's C code and Zig libraries
 */

#include <stddef.h>
#include <stdbool.h>
#include "goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===== Memory Management =====

/**
 * Initialize Zig memory subsystem
 */
bool goo_zig_memory_init(void);

/**
 * Clean up Zig memory subsystem
 */
void goo_zig_memory_cleanup(void);

/**
 * Allocate aligned memory
 */
void* goo_zig_alloc_aligned(size_t size, size_t alignment);

/**
 * Reallocate aligned memory
 */
void* goo_zig_realloc_aligned(void* ptr, size_t old_size, size_t new_size, size_t alignment);

/**
 * Free aligned memory
 */
void goo_zig_free_aligned(void* ptr, size_t size, size_t alignment);

/**
 * Copy aligned memory
 */
void goo_zig_copy_aligned(void* dest, const void* src, size_t size, size_t alignment);

/**
 * Set aligned memory
 */
void goo_zig_set_aligned(void* dest, uint8_t value, size_t size, size_t alignment);

// ===== SIMD Vectorization =====

/**
 * Initialize vectorization subsystem
 */
bool goo_zig_vectorization_init(int simd_type);

/**
 * Clean up vectorization subsystem
 */
void goo_zig_vectorization_cleanup(void);

/**
 * Detect available SIMD type
 */
int goo_zig_vectorization_detect_simd(void);

/**
 * Get required alignment for SIMD operations
 */
size_t goo_zig_vectorization_get_alignment(int simd_type);

/**
 * Check if pointer is aligned for SIMD operations
 */
bool goo_zig_vectorization_is_aligned(void* ptr, int simd_type);

/**
 * Get vector width for data type with SIMD
 */
size_t goo_zig_vectorization_get_width(int data_type, int simd_type);

/**
 * Check if operation can be accelerated with SIMD
 */
bool goo_zig_vectorization_is_accelerated(int data_type, int op, int simd_type);

/**
 * Create a vector mask
 */
void* goo_zig_vectorization_create_mask(size_t size, int type_val);

/**
 * Free a vector mask
 */
void goo_zig_vectorization_free_mask(void* mask);

#ifdef __cplusplus
}
#endif

#endif // GOO_ZIG_RUNTIME_H 