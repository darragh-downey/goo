#ifndef GOO_MEMORY_H
#define GOO_MEMORY_H

#include <stddef.h>

/**
 * Goo Memory Management - Legacy API
 * This header maintains compatibility with older code
 * while forwarding to the new memory management API
 */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for the legacy memory API

/**
 * Allocate memory from the default allocator
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* goo_runtime_alloc(size_t size);

/**
 * Reallocate previously allocated memory
 * @param ptr Pointer to previously allocated memory
 * @param new_size New size for the memory block
 * @return Pointer to reallocated memory or NULL on failure
 */
void* goo_runtime_realloc(void* ptr, size_t new_size);

/**
 * Free previously allocated memory
 * @param ptr Pointer to memory allocated by goo_runtime_alloc
 */
void goo_runtime_free(void* ptr);

// Legacy aliases (maintained for backward compatibility)
void* goo_alloc(size_t size);
void* goo_realloc(void* ptr, size_t size);
void goo_free(void* ptr);

#ifdef __cplusplus
}
#endif

// For additional memory functions, include the modern API
#include "goo/runtime/memory.h"

#endif /* GOO_MEMORY_H */ 