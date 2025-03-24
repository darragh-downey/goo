#ifndef GOO_ALLOCATOR_H
#define GOO_ALLOCATOR_H

/**
 * Memory allocator definitions for Goo language
 */

#include <stddef.h>
#include <stdbool.h>
#include "goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Allocator callback function types
typedef void* (*GooAllocFn)(void* context, size_t size, size_t alignment);
typedef void* (*GooReallocFn)(void* context, void* ptr, size_t old_size, size_t new_size, size_t alignment);
typedef void (*GooFreeFn)(void* context, void* ptr, size_t size);

// Complete definition of allocator structure
struct GooAllocator {
    // Allocator type
    GooAllocatorType type;
    
    // Allocator context (implementation-specific)
    void* context;
    
    // Memory operations
    GooAllocFn alloc;
    GooReallocFn realloc;
    GooFreeFn free;
};

// ===== Standard allocator functions =====

/**
 * Create a new standard heap allocator
 */
GooAllocator* goo_create_heap_allocator(void);

/**
 * Free a standard heap allocator
 */
void goo_free_heap_allocator(GooAllocator* allocator);

/**
 * Get the default system allocator
 */
GooAllocator* goo_get_default_allocator(void);

/**
 * Allocate memory using an allocator
 */
void* goo_alloc(GooAllocator* allocator, size_t size, size_t alignment);

/**
 * Reallocate memory using an allocator
 */
void* goo_realloc(GooAllocator* allocator, void* ptr, size_t old_size, size_t new_size, size_t alignment);

/**
 * Free memory using an allocator
 */
void goo_free(GooAllocator* allocator, void* ptr, size_t size);

// Default allocation functions for standard allocators
void* default_alloc(void* context, size_t size, size_t alignment);
void* default_realloc(void* context, void* ptr, size_t old_size, size_t new_size, size_t alignment);
void default_free(void* context, void* ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif // GOO_ALLOCATOR_H 